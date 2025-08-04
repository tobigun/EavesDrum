// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config/config_mapper.h"
#include "drum_kit.h"
#include "event_log.h"
#include "log.h"
#include "midi_device.h"
#include "network.h"
#include "trigger_button.h"
#include "usb.h"
#include "version.h"
#include "webui.h"
#include "wifi_connect.h"

#include <Arduino.h>

MidiDevice MIDI;
NetworkConnection network;

DrumKit drumKit;

void logVersion() {
  eventLog.log(Level::INFO, String("Version: ") + Version::getPackageVersion()
    + " (Git: " + Version::getGitCommitHash() + ", Date: " + Version::getBuildTime() + ")");
}

#ifdef HAS_BLUETOOTH
void onBluetoothConnected() {
  DrumIO::led(LED_NETWORK, true); // TODO: change led id
}

void onBluetoothDisconnected() {
  DrumIO::led(LED_NETWORK, false); // TODO: change led id
}
#endif

#ifdef USE_WIFI
static WifiConnect wifi;

void initWireless() {
  EDRUM_DEBUGLN("Connecting ...");
  DrumIO::led(LED_NETWORK, false);
  wifi.connect();
  // wifi.startServer();
  DrumIO::led(LED_NETWORK, true);
  EDRUM_DEBUGLN("Connected");
}

#endif

void setup() {
#ifdef ENABLE_SERIAL
  SerialDebug.begin(115200);
#endif

  logVersion();

  // Note: config must be loaded before USB is started, otherwise USB will not initialize correctly
  DrumConfigMapper::loadAndApplyDrumKitConfig(drumKit);

  MIDI.begin();
#ifdef USE_WIFI
  triggerButton.init();
  initWireless();
#else
  setupUsb();
#endif

  DrumIO::setup(true);

  network.setup();
  webUI.setup(drumKit);

#if ENABLE_MASS_STORAGE
  enableMassStorageDevice();
#endif

  // setupTouch();
}

static void blinkLed() {
  static uint32_t last_time = 0;
  static int led_state = HIGH;
  uint32_t cur_time = millis();

  if (cur_time - last_time > 1000) {
    led_state = !led_state;
    DrumIO::led(LED_WATCHDOG, led_state);
    last_time = cur_time;
  }
}

static void updateWifiState() {
#ifdef USE_WIFI
  if (triggerButton.isPressed()) {
    DrumIO::led(LED_NETWORK, false);
    wifi.provision();
    DrumIO::led(LED_NETWORK, true);
  }
#endif
}

void loop() {
  blinkLed();

  drumKit.updateDrums();

  updateWifiState();
  network.service_traffic();

  // touchSense();
}
