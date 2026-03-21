// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config/config_mapper.h"
#include "drum_kit.h"
#include "event_log.h"
#include "log.h"
#include "midi_transport.h"
#include "network_connection.h"
#include "usb.h"
#include "version.h"
#include "webui.h"

#include <Arduino.h>

DrumKit drumKit;

static void logVersion();
static void ledTest();
static void blinkLed();

void setup() {  
  DrumIO::setup(true);

#ifdef ENABLE_SERIAL_DEBUG
  SerialDebug.begin(LOG_BAUD);
#endif

  ledTest();

  logVersion();

  // Note: config must be loaded before USB is started, otherwise USB will not initialize correctly
  DrumConfigMapper::loadAndApplyDrumKitConfig(drumKit);

  midiTransport.begin();

  setupUsb();

  networkConnection.begin();
  webUI.setup(drumKit);

#if ENABLE_MASS_STORAGE
  enableMassStorageDevice();
#endif

  midiTransport.update();

  // setupTouch();
}

void loop() {
  blinkLed();

  drumKit.updateDrums();

  networkConnection.update();
  midiTransport.update();

  DrumIO::resetWatchdog();

  // touchSense();
}

static void logVersion() {
  eventLog.log(Level::Info, String("Version: ") + Version::getPackageVersion()
    + " (Git: " + Version::getGitCommitHash() + ", Date: " + Version::getBuildTime() + ")");
}

static void ledTest() {
  DrumIO::led(LedId::HitIndicator, true);
  DrumIO::led(LedId::Network, true);
  DrumIO::led(LedId::MidiConnected, true);
  DrumIO::led(LedId::WatchDog, true);

  delay(200);

  DrumIO::led(LedId::WatchDog, false);
  DrumIO::led(LedId::HitIndicator, false);
  DrumIO::led(LedId::Network, false);
  DrumIO::led(LedId::MidiConnected, false);
}

static void blinkLed() {
  static uint32_t last_time = 0;
  static int led_state = HIGH;
  uint32_t cur_time = millis();

  if (cur_time - last_time > 1000) {
    led_state = !led_state;
    DrumIO::led(LedId::WatchDog, led_state);
    last_time = cur_time;
  }
}
