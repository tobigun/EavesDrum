// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi_transport_guitar_hero_wii.h"

#ifdef ENABLE_MIDI_GUITAR_HERO_WII_TRANSPORT

#include <Arduino.h>
#include <wiimote.h>
#include <wiimote_btstack.h>
#include "drum_io.h"
#include "log.h"
#include "packed.h"
#include "usb_host_gamepad.h"
#include "wii_client.h"
#include "config/config_mapper.h"
#include "drum_kit.h"
#include "guitar_hero_util.h"

#define SAVE_DELAY_MS 5000 // 5s

#define HIT_HOLD_TIME_MS 20 // original: ~50ms
#define MIDI_CHANNEL 10

WiiClient wiiClient;

static WiiClientPairingInfo pairingInfo;

enum class PadHitState {
  Unpressed,
  Pressed,
  Released
};

uint32_t hitTimesMs[NUM_PADS] = {0};

static WiimoteReport wii_report;

uint32_t configNeedsSaveTimeMs = 0;

static void wiimote_setLed(bool on) {
  DrumIO::led(LedId::MidiConnected, on);
}

static String hexToString(uint8_t* buffer, uint8_t len) {
  char hexbuf[3];
  String result = "";
  for (uint8_t i = 0; i < len; ++i) {
    sprintf(hexbuf, "%02X", buffer[i]);
    result += String(hexbuf) + ((i != len-1) ? ":" : "");
  }
  return result;
}

static void stringToHex(String str, uint8_t* outBuffer) {
  for (uint8_t i = 0; i < str.length(); i += 3) {
    String hex = str.substring(i, i + 2);
    outBuffer[i / 3] = strtol(hex.c_str(), NULL, 16);
  }
}

static void wiimote_connected(wii_addr_t wii_addr, wii_link_key_t link_key) {
  String wiiAddrString = hexToString(wii_addr, sizeof(wii_addr_t));
  String linkKeyString = hexToString(link_key, sizeof(wii_link_key_t));

  if (wiiAddrString != pairingInfo.address || linkKeyString != pairingInfo.linkKey) {
    pairingInfo.address = wiiAddrString;
    pairingInfo.linkKey = linkKeyString;
    configNeedsSaveTimeMs = millis();
  }

  logInfo("Wii connected%s: %s link-key: %s\n", (configNeedsSaveTimeMs > 0 ? " (save)" : ""),
    wiiAddrString.c_str(), linkKeyString.c_str());
}

static void triggerPadHit(uint8_t padId, uint8_t velocity, uint8_t note) {
  wiimote_midi& midi = wii_report.midi;
  midi.channel = MIDI_CHANNEL - 1;
  midi.velocity = velocity;
  midi.note = note;

  hitTimesMs[padId] = millis();
}

#define LS_MAX 63
#define LS_CENTER 32
#define LS_DEADZONE 2

static void onGamepadReportReceived(const USB_Host_Data_t& data) {
  wiimote_buttons& buttons = wii_report.wiimote;
  wiimote_classic& classic = wii_report.classic;

  classic.ls_x = LS_CENTER;
  classic.ls_y = LS_CENTER;

  bool hasAnalogStick = data.hasAxisX && data.hasAxisY;
  if (hasAnalogStick) {
    uint8_t ls_x = data.genericAxisX >> 10;
    if (ls_x < (LS_CENTER - LS_DEADZONE) || ls_x > (LS_CENTER + LS_DEADZONE)) {
      classic.ls_x = data.genericAxisX >> 10;
    }

    uint8_t ls_y = LS_MAX - (data.genericAxisY >> 10);
    if (ls_y < (LS_CENTER - LS_DEADZONE) || ls_y > (LS_CENTER + LS_DEADZONE)) {
      classic.ls_y = LS_MAX - (data.genericAxisY >> 10);
    }
  }

  if (data.hasDPad) {
    classic.up = buttons.up = data.dpadUp;
    classic.down = buttons.down = data.dpadDown;
    classic.left = buttons.left = data.dpadLeft;
    classic.right = buttons.right = data.dpadRight;

    // GH and Rockband only support the analog stick although the digital stick would be much nicer for the menus.
    // -> Overwrite analog stick if digital stick was pressed
    if (data.dpadLeft) {
      classic.ls_x = 10;
    }
    if (data.dpadRight) {
      classic.ls_x = LS_MAX - 10;
    }
    if (data.dpadUp) {
      classic.ls_y = LS_MAX - 10;
    }
    if (data.dpadDown) {
      classic.ls_y = 10;
    }
  }

  uint8_t note = 0;
  buttons.a = false;
  if (data.genericButton1) {
    note = GH_YELLOW_NOTE_DEFAULT;
  } else if (data.genericButton2) {
    note = GH_RED_NOTE_DEFAULT;
  } else if (data.genericButton3) {
    buttons.a = data.genericButton3;
    note = GH_GREEN_NOTE_DEFAULT;
  } else if (data.genericButton4) {
    note = GH_BLUE_NOTE_DEFAULT;
  }

  static bool wasPressed = false;
  if (note && !wasPressed) {
    wasPressed = true;
    uint8_t padId = noteToPadId(note);
    triggerPadHit(padId, 100, note);
  } else if (!note && wasPressed) {
    wasPressed = false;
  }

  classic.minus = buttons.minus = data.genericButton5;
  classic.plus = buttons.plus = data.genericButton6;
  classic.home = buttons.home = data.genericButton13; // 13 is "PS" on PS controller
}

static PadHitState isPadHit(uint8_t padId, uint32_t currentTimeMs) {
  uint32_t& hitTimeMs = hitTimesMs[padId];
  if (hitTimeMs == 0) {
    return PadHitState::Unpressed;
  }
  
  if (currentTimeMs - hitTimeMs > HIT_HOLD_TIME_MS) {
    hitTimeMs = 0;
    return PadHitState::Released;
  }

  return PadHitState::Pressed;
}

static void setClassicButton(uint8_t padId, bool pressed) {
  wiimote_classic& classic = wii_report.classic;
  switch (padId) {
  case GH_Red: classic.b = pressed; break;
  case GH_Blue: classic.x = pressed; break;
  case GH_Green: classic.a = pressed; break;
  case GH_Yellow: classic.y = pressed; break;
  case GH_Orange: classic.lz = pressed; break;
  case GH_Kick: classic.rz = pressed; break;
  }
}

static bool wiimote_set_report(WiimoteReport* report) {
  uint32_t currentTimeMs = millis();
  for (uint8_t padId = 0; padId < NUM_PADS; ++padId) {
    PadHitState hitState = isPadHit(padId, currentTimeMs);
    if (hitState == PadHitState::Pressed) {
      setClassicButton(padId, true);
    } else if (hitState == PadHitState::Released) {
      setClassicButton(padId, false);
    }
  }

  bool changed = memcmp(report, &wii_report, sizeof(wii_report)) != 0;
  memcpy(report, &wii_report, sizeof(wii_report));

  // midi information is only sent in first report after hit
  memset(&wii_report.midi, 0, sizeof(wiimote_midi));

  return changed;
}

void MidiTransport_GuitarHero_Wii::start() {
  DrumIO::led(LedId::MidiConnected, false);

  memset(&wii_report, 0, sizeof(wii_report));
  wii_report.extension = GuitarHeroDrum;
  wii_report.reset_ir = 1; // required to change extension type

  WiimoteConfig config = {
    .has_speaker = false, // no speaker -> no sound data is transferred
    .has_wmp = false, // no motion plus

    .report_callback = wiimote_set_report,
    .set_led_state_callback = wiimote_setLed,
    .connection_callback = wiimote_connected
  };

  if (pairingInfo.address.isEmpty()) {
    memset(config.wii_addr, 0, sizeof(wii_addr_t)); 
    memset(config.wii_link_key, 0, sizeof(wii_link_key_t)); 
  } else {
    wii_addr_t wii_addr;
    wii_link_key_t link_key;
    stringToHex(pairingInfo.address, wii_addr);
    stringToHex(pairingInfo.linkKey, link_key);
    memcpy(config.wii_addr, wii_addr, sizeof(wii_addr_t)); 
    memcpy(config.wii_link_key, link_key, sizeof(wii_link_key_t)); 
  }

  wiimote_emulator_init(&config);

#ifdef ENABLE_TINY_USB_HOST_GAMEPAD
  UsbHostGamepad::start(onGamepadReportReceived);
#endif
}

void MidiTransport_GuitarHero_Wii::stop() {
  DrumIO::led(LedId::MidiConnected, false);
  wiimote_emulator_deinit();
}

void MidiTransport_GuitarHero_Wii::update() {
#ifdef ENABLE_TINY_USB_HOST_GAMEPAD
  UsbHostGamepad::update();
#endif

  if (configNeedsSaveTimeMs > 0 && millis() - configNeedsSaveTimeMs > SAVE_DELAY_MS) {
    logInfo("Save config and reset");
    wiimote_emulator_deinit();
    DrumConfigMapper::saveDrumKitConfig(drumKit);
    // saving the config takes too long and seems to corrupts the bluetooth stack (no button presses are registered).
    // In addition to the save delay, we have to reboot here to get the stack into a working state again
    DrumIO::requestReset(100);
    configNeedsSaveTimeMs = 0;
  }

  if (DrumIO::isButtonPressed(ButtonId::Button1)) {
    wiimote_emulator_pair_remote(PAIRING_SYNC);
  }
#if 0
  if (DrumIO::isButtonPressed(ButtonId::Button2)) {
    wiimote_emulator_pair_remote(PAIRING_GUEST);
  }
#endif
}

void MidiTransport_GuitarHero_Wii::sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
  int8_t padId = noteToPadId(inNoteNumber);
  if (padId < 0) {
    return;
  }

  triggerPadHit(padId, inVelocity, padIndexToNote(padId));
}

void MidiTransport_GuitarHero_Wii::sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) {
}


const WiiClientPairingInfo& WiiClient::getPairingInfo() const {
  return pairingInfo;
}

void WiiClient::setPairingInfo(const String& address, const String& linkKey) {
  pairingInfo.address = address;
  pairingInfo.linkKey = linkKey;
}

#endif
