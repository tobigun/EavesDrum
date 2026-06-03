// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

// Uses information from:
// - PlasticBand Instrument Peripheral Documentation by TheNathannator (Nathan), Sanjay Govind, et al.
//   Github: https://github.com/TheNathannator/PlasticBand/blob/main/Docs/Instruments/4-Lane%20Drums/PS3%20and%20Wii.md
//   Creative Commons Attribution-ShareAlike 4.0 International License
// - Rock Band MIDI-to-USB Drum Controller for Nintendo Wii from Ken Kurzweil
//   Github: https://github.com/kurzweil/MidiRock
//   MIT License (Copyright: Rock Band MIDI-to-USB Drum Controller Contributors)  

#include "midi_transport_rockband.h"

#ifdef ENABLE_MIDI_ROCKBAND

#include "drum_io.h"
#include "log.h"
#include "packed.h"
#include "usb_host_gamepad.h"
#include "usb_device.h"

#include <Arduino.h>
#include <class/hid/hid.h>
#include <class/hid/hid_device.h>
#include <tusb.h>

//https://github.com/DarkRTA/rb3/blob/master/src/system/usbwii/UsbWii.cpp#L61
static const uint16_t usbVendorId = 0x1bad; // Harmonix Music Systems / Mad Catz vendor ID
static const uint16_t usbProductId = 0x3110; // RB2 Drums, RB1 Drums (0x0005), MIDI Pro Adapter, Drums (0x3138)

static uint16_t origVendorId;
static uint16_t origProductId;

static const char* HID_NAME_WII = "Harmonix Drum Controller for Nintendo Wii";
static const char* HID_NAME_GENERIC = "EavesDrum Controller";

static const uint8_t HID_DESCRIPTOR[] = {
    HID_USAGE_PAGE(HID_USAGE_PAGE_DESKTOP), /* Generic Desktop */
    HID_USAGE(HID_USAGE_DESKTOP_GAMEPAD), /* Game Pad */
    HID_COLLECTION(HID_COLLECTION_APPLICATION), /* Application */
    HID_LOGICAL_MIN(0x00),
    HID_LOGICAL_MAX(0x01),
    HID_PHYSICAL_MIN(0x00),
    HID_PHYSICAL_MAX(0x01),
    HID_REPORT_SIZE(0x01),
    HID_REPORT_COUNT(0x0D),
    HID_USAGE_PAGE(0x09), /* Button */
    HID_USAGE_MIN(0x01),
    HID_USAGE_MAX(0x0D),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_REPORT_COUNT(0x03),
    HID_INPUT(HID_CONSTANT),
    HID_USAGE_PAGE(0x01), /* Generic Desktop */
    HID_LOGICAL_MAX(0x07),
    HID_PHYSICAL_MAX_N(0x013B, 2),
    HID_REPORT_SIZE(0x04),
    HID_REPORT_COUNT(0x01),
    HID_UNIT(0x14), /* Rotation (Eng. Pos) */
    HID_USAGE(0x39), /* Hat switch */
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE | HID_NULL_STATE),
    HID_UNIT(0x00),
    HID_REPORT_COUNT(0x01),
    HID_INPUT(HID_CONSTANT),
    HID_LOGICAL_MAX_N(0x00FF, 2),
    HID_PHYSICAL_MAX_N(0x00FF, 2),
    HID_USAGE(0x30), /* X */
    HID_USAGE(0x31), /* Y */
    HID_USAGE(0x32), /* Z */
    HID_USAGE(0x35), /* Rz */
    HID_REPORT_SIZE(0x08),
    HID_REPORT_COUNT(0x04),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_USAGE_PAGE_N(0xFF00, 2), /* Vendor-defined */
    HID_USAGE(0x20),
    HID_USAGE(0x21),
    HID_USAGE(0x22),
    HID_USAGE(0x23),
    HID_USAGE(0x24),
    HID_USAGE(0x25),
    HID_USAGE(0x26),
    HID_USAGE(0x27),
    HID_USAGE(0x28),
    HID_USAGE(0x29),
    HID_USAGE(0x2A),
    HID_USAGE(0x2B),
    HID_REPORT_COUNT(0x0C),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_USAGE_N(0x2621, 2), /* Vendor-defined */
    HID_REPORT_COUNT(0x08),
    HID_FEATURE(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_USAGE_N(0x2621, 2), /* Vendor-defined */
    HID_OUTPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_LOGICAL_MAX_N(0x03FF, 2),
    HID_PHYSICAL_MAX_N(0x03FF, 2),
    HID_USAGE(0x2C),
    HID_USAGE(0x2D),
    HID_USAGE(0x2E),
    HID_USAGE(0x2F),
    HID_REPORT_SIZE(0x10),
    HID_REPORT_COUNT(0x04),
    HID_INPUT(HID_DATA | HID_VARIABLE | HID_ABSOLUTE),
    HID_COLLECTION_END,
};

#define HIT_HOLD_TIME_MS 20 // hold button active

enum HatPosition {
  HAT_UP = 0,
  HAT_UP_RIGHT = 1,
  HAT_RIGHT = 2,
  HAT_DOWN_RIGHT = 3,
  HAT_DOWN = 4,
  HAT_DOWN_LEFT = 5,
  HAT_LEFT = 6,
  HAT_UP_LEFT = 7,
  HAT_CENTERED = 8
};

struct ATTR_PACKED RBButtons_t {
  union {
    struct {
      bool blue : 1; // square / 1
      bool green : 1; // cross / A
      bool red : 1; // circle / B
      bool yellow : 1; // triangle / 2

      bool kick : 1; // l1 / Kick1 Orange
      bool hiHatPedal : 1; // r1 / Kick2 Black
      bool genericBlueCymbal: 1; // l2 (unused by Rockband)
      bool genericGreenCymbal: 1; // r2 (unused by Rockband)

      bool select : 1; // minus
      bool start : 1; // plus
      #define genericYellowCymbal isPadHit
      bool isPadHit : 1; // l3 (generic Yellow Cymbal)
      bool isCymbalHit : 1; // r3

      bool home : 1; // ps
      bool : 3;
    };
    uint16_t bits;
  };
};

struct ATTR_PACKED RBVelocities_t {
  uint8_t yellow;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t pedal; // GH only
  uint8_t orange; // GH only
};

struct ATTR_PACKED HIDReport_t {
  RBButtons_t buttons;

  uint8_t hat;
  
  uint8_t axes[4]; // X, Y, Z, Rz
  
  uint8_t unused1[4]; // unused presure values

  RBVelocities_t velocities;

  uint8_t unused2[2]; // unused presure values
  uint16_t unused3[4]; // unused accelerometer values
};

struct HitInfo {
  uint32_t timeMs = 0;
  uint8_t velocity = 0;
};

#define NUM_PADS 9
static HitInfo hitInfos[NUM_PADS];

static bool isGenericGamepadMode = true;

enum class PadHitState {
  Unpressed,
  Pressed,
  Released
};
enum RBDrumPadId {
  RB_Red_Pad,
  RB_Yellow_Pad,
  RB_Blue_Pad,
  RB_Green_Pad,
  RB_Yellow_Cymbal,
  RB_Blue_Cymbal,
  RB_Green_Cymbal,
  RB_Kick,
  RB_HiHatPedal
};

// See Mad-Catz Rock Band 3 Midi Pro-Adapter (Wii)
static int8_t noteToPadId(uint8_t noteNumber) {
  switch (noteNumber) {
  
  // Pads
  
  case 38: // D1 (Acoustic Snare)
  case 31: // G0 (n.a.)
  case 34: // A#0 (n.a.)
  case 37: // C#1 (Side Stick)
  case 39: // D#1 (Hand Clap)
  case 40: // E1 (Electric Snare)
    return RB_Red_Pad; // Snare

  case 48: // C2 (Hi-Mid Tom)
  case 50: // D2 (High Tom)
    return RB_Yellow_Pad; // Hi-tom

  case 45: // A1 (Low Tom)
  case 47: // B1 (Low-Mid Tom)
    return RB_Blue_Pad; // Low-tom

  case 41: // F1 (Low Floor Tom)
  case 43: // G1 (High Floor Tom)
    return RB_Green_Pad; // Floor-tom

  // Cymbals

  case 22: // A#-1 (n.a.)
  case 26: // D0 (n.a.)
  case 42: // F#1 (Closed Hi-Hat)
  case 46: // A#1 (Open Hi-Hat)
  case 54: // F#2 (Tambourine)
    return RB_Yellow_Cymbal; // Hi-Hat Cymbal

  case 51: // D#2 (Ride Cymbal 1)
  case 53: // F2 (Ride Bell)
  case 56: // G#2 (Cowbell)
  case 59: // B2 (Ride Cymbal 2)
    return RB_Blue_Cymbal; // Ride Cymbal

  case 49: // C#2 (Crash Cymbal 1)
  case 52: // E2 (Chinese Cymbal)
  case 55: // G2 (Splash Cymbal)
  case 57: // A2 (Crash Cymbal 2)
    return RB_Green_Cymbal; // Crash Cymbal

  // Pedals

  case 33: // A0 (n.a.)
  case 35: // B0 (Acoustic Bass Drum)
  case 36: // C1 (Bass Drum 1)
    return RB_Kick; // Kick Pedal

  case 44: // G#1 (Pedal Hi-Hat)
    return RB_HiHatPedal; // Hi-Hat Pedal

  default:
    return -1;
  }
}

static HIDReport_t hidReport = {
    .buttons = 0x0000,
    .hat = 8,
    .axes = {0x7F, 0x7F, 0x7F, 0x7F},
    .velocities = {0, 0, 0, 0, 0, 0},
    .unused3 = {0, 0, 0, 0}};

static void onGamepadReportReceived(const USB_Host_Data_t& data) {
  hidReport.hat = HAT_CENTERED;
  if (data.hasDPad) {
    if (data.dpadUp) {
      hidReport.hat = HAT_UP;
    } else if (data.dpadDown) {
      hidReport.hat = HAT_DOWN;
    } else if (data.dpadLeft) {
      hidReport.hat = HAT_LEFT;
    } else if (data.dpadRight) {
      hidReport.hat = HAT_RIGHT;
    }
  } else if (data.hasAxisX && data.hasAxisY) {
    uint8_t x = data.genericAxisX >> 8;
    uint8_t y = data.genericAxisY >> 8;
    if (x < 50) {
      hidReport.hat = HAT_LEFT;
    } else if (x > UINT8_MAX - 50) {
      hidReport.hat = HAT_RIGHT;
    } else if (y < 50) {
      hidReport.hat = HAT_UP;
    } else if (y > UINT8_MAX - 50) {
      hidReport.hat = HAT_DOWN;
    }
  }

  hidReport.buttons.blue = data.genericButton1;
  hidReport.buttons.red = data.genericButton2;
  hidReport.buttons.green = data.genericButton3;
  hidReport.buttons.yellow = data.genericButton4;
  hidReport.buttons.select = data.genericButton5 || data.genericButton9; // 9 is "select" on PS controller
  hidReport.buttons.start = data.genericButton6 || data.genericButton10; // 10 is "start" on PS controller
  hidReport.buttons.home = data.genericButton7 || data.genericButton13; // 13 is "PS" on PS controller
}

void restartUsbDevice() {
  if (UsbDevice::isInited()) {
    logInfo("USB device already inited -> restart");
    UsbDevice::end();
    UsbDevice::begin();
  }
}

void MidiTransport_Rockband::start(MidiOutputMode mode) {
  logInfo("Starting Rockband");
  DrumIO::led(LedId::MidiConnected, true);

  isGenericGamepadMode = (mode == MidiOutputMode::GamepadUsb);

  origVendorId = UsbDevice::getVendorId();
  origProductId = UsbDevice::getProductId();

  if (!isGenericGamepadMode) {
    UsbDevice::setVendorId(usbVendorId);
    UsbDevice::setProductId(usbProductId);
  } else {
    // use different product id as Windows caches USB device info
    UsbDevice::setProductId(UsbDevice::getProductId() + 1);
  }

  const char* hidName = isGenericGamepadMode ? HID_NAME_GENERIC : HID_NAME_WII;
  UsbDevice::enableHid(hidName, HID_DESCRIPTOR, sizeof(HID_DESCRIPTOR), 1);
  restartUsbDevice();

  hidReport.hat = HAT_CENTERED;

#ifdef ENABLE_TINY_USB_HOST_GAMEPAD
  UsbHostGamepad::start(onGamepadReportReceived);
#endif
}

void MidiTransport_Rockband::stop() {
  DrumIO::led(LedId::MidiConnected, false);

  UsbDevice::setVendorId(origVendorId);
  UsbDevice::setProductId(origProductId);

  UsbDevice::disableHid();
  restartUsbDevice();
}

static PadHitState isPadHit(uint8_t padId, uint32_t currentTimeMs) {
  uint32_t hitTimeMs = hitInfos[padId].timeMs;
  if (hitTimeMs == 0) {
    return PadHitState::Unpressed;
  }
  
  if (currentTimeMs - hitTimeMs > HIT_HOLD_TIME_MS) {
    hitInfos[padId].timeMs = 0;
    hitInfos[padId].velocity = 0;
    return PadHitState::Released;
  }

  return PadHitState::Pressed;
}

static bool isPad(uint8_t padId) {
  switch (padId) {
  case RB_Red_Pad:
  case RB_Yellow_Pad:
  case RB_Blue_Pad:
  case RB_Green_Pad:
    return true;
  default:
    return false;
  }
}

static bool isCymbal(uint8_t padId) {
  switch (padId) {
  case RB_Yellow_Cymbal:
  case RB_Blue_Cymbal:
  case RB_Green_Cymbal:
    return true;
  default:
    return false;
  }
}

static void updateHidCymbalVelocity(uint8_t& cymbalVelocity, uint8_t value, RBVelocities_t& velocities) {
  // if pad and cymbal of same color are hit at the same time, use red velocity for cymbal
  if (cymbalVelocity == 0) {
    cymbalVelocity = value;
  } else {
    velocities.red = value;
  }
}

static void updateHidPadInfo(
  uint8_t padId,
  uint8_t velocity,
  RBButtons_t& buttons,
  RBVelocities_t& velocities,
  uint8_t& hat
) {
  switch (padId) {
  case RB_Red_Pad:
    buttons.red = true;
    velocities.red = velocity;
    break;
  case RB_Yellow_Pad:
    buttons.yellow = true;
    velocities.yellow = velocity;
    break;
  case RB_Blue_Pad:
    buttons.blue = true;
    velocities.blue = velocity;
    break;
  case RB_Green_Pad:
    buttons.green = true;
    velocities.green = velocity;
    break;
  case RB_Yellow_Cymbal:
    if (isGenericGamepadMode) {
      buttons.genericYellowCymbal = true;
    } else {
      buttons.yellow = true;
      hat = HAT_UP;
    }
    updateHidCymbalVelocity(velocities.yellow, velocity, velocities);
    break;
  case RB_Blue_Cymbal:
    if (isGenericGamepadMode) {
      buttons.genericBlueCymbal = true;
    } else {
      buttons.blue = true;
      hat = HAT_DOWN;
    }
    updateHidCymbalVelocity(velocities.blue, velocity, velocities);
    break;
  case RB_Green_Cymbal:
    if (isGenericGamepadMode) {
      buttons.genericGreenCymbal = true;
    } else {
      buttons.green = true;
    }
    updateHidCymbalVelocity(velocities.green, velocity, velocities);
    break;
  case RB_Kick:
    buttons.kick = true;
    break;
  case RB_HiHatPedal:
    buttons.hiHatPedal = true;
    break;
  }
}

static void updateHidReport() {
  if (!tud_hid_ready()) {
    return;
  }

  // directly work on velocities from HID report as USB-Host gamepad does not use it
  hidReport.velocities = {0, 0, 0, 0, 0, 0};

  // work on copies of button and hat state to not overwrite USB-Host gamepad input
  RBButtons_t buttons = { .bits = 0 };
  uint8_t hat = HAT_CENTERED;
  bool hasDrumEvent = false;

  uint32_t currentTimeMs = millis();
  for (uint8_t padId = 0; padId < NUM_PADS; ++padId) {
    PadHitState hitState = isPadHit(padId, currentTimeMs);
    if (hitState == PadHitState::Pressed) {
      updateHidPadInfo(padId, hitInfos[padId].velocity, buttons, hidReport.velocities, hat);
      if (!isGenericGamepadMode) {
        if (isPad(padId)) {
          buttons.isPadHit = true;
        } else if (isCymbal(padId)) {
          buttons.isCymbalHit = true;
        }
      }
      hasDrumEvent = true;
    } else if (hitState == PadHitState::Released) {
      hasDrumEvent = true;
    }
  }

  if (hasDrumEvent) { // only overwrite gamepad input on drum event
    hidReport.buttons = buttons;
    hidReport.hat = hat;
  }

  tud_hid_report(0, (uint8_t*)&hidReport, sizeof(HIDReport_t));
}

void MidiTransport_Rockband::update() {
#ifdef ENABLE_TINY_USB_HOST_GAMEPAD
  UsbHostGamepad::update();
#endif

  updateHidReport();
}

static void removeOldestHit() {
  int8_t oldestPadId = 0;
  uint32_t oldestHitTimeMs = UINT32_MAX;

  for (int padId = 0; padId < NUM_PADS; ++padId) {
    uint32_t hitTimeMs = hitInfos[padId].timeMs;
    if (hitTimeMs > 0 && hitTimeMs < oldestHitTimeMs) {
      oldestPadId = padId;
      oldestHitTimeMs = hitTimeMs;
    }
  }

  hitInfos[oldestPadId].timeMs = 0;
  hitInfos[oldestPadId].velocity = 0;
}

static bool hasConflict(uint8_t newPadId) {
  bool newIsPad = isPad(newPadId);
  bool newIsCymbal = isCymbal(newPadId);
  if (!newIsPad && !newIsCymbal) { // is pedal
    return false; // ok
  }

  uint8_t numPads = 0;
  uint8_t numCymbals = 0;
  for (int i = 0; i < NUM_PADS; ++i) {
    bool hit = hitInfos[i].timeMs > 0;
    if (hit && isPad(i)) {
      ++numPads;
    } else if (hit && isCymbal(i)) {
      ++numCymbals;
    }
  }

  if (newIsPad && numCymbals == 0) { // all pads
    return false; // ok
  } else if (newIsCymbal && numPads == 0) { // all cymbals
    return false; // ok
  } else if (newIsPad && numPads == 0 && numCymbals == 1) { // max. one pad and one cymbal
    return false; // ok
  } else if (newIsCymbal && numCymbals == 0 && numPads == 1) { // max. one pad and one cymbal
    return false; // ok
  }

  return true; // conflict detected
}

void MidiTransport_Rockband::sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
  int8_t padId = noteToPadId(inNoteNumber);
  if (padId < 0) {
    return;
  }

  if (hitInfos[padId].timeMs > 0) {
    return; // ignore hits during hold time
  }

  while (hasConflict(padId)) {
    removeOldestHit();
  }

  hitInfos[padId] = {
    .timeMs = millis(),
    .velocity = inVelocity
  };
}

void MidiTransport_Rockband::sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) {
  // TODO: handle CC#4 (supported by Rock Band MIDI Pro-Adapter)
}

#endif
