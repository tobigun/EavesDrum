// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config/config_mapper.h"
#include "drum_kit.h"
#include "event_log.h"
#include "log.h"
#include "midi_transport.h"
#include "network.h"
#include "trigger_button.h"
#include "usb.h"
#include "version.h"
#include "webui.h"
#include "wifi_connect.h"

#ifdef HAS_BLUETOOTH
#include "ble_client.h"
#include "ble_server.h"
#endif

#if __has_include(<tusb.h>)
#include <tusb.h>
#define HAS_TINY_USB
#endif

#include <Arduino.h>

NetworkConnection network;

DrumKit drumKit;

#ifdef USE_WIFI
static WifiConnect wifi;
#endif

static void logVersion();
static void blinkLed();
static void ledTest();
static void reconnectUsb();
static void updateBluetooth();

static void initWifi();
static void updateWifiState();

void setup() {  
  DrumIO::setup(true);

  reconnectUsb();

#ifdef ENABLE_SERIAL_DEBUG
  SerialDebug.begin(115200);
#endif

  ledTest();

  logVersion();

  // Note: config must be loaded before USB is started, otherwise USB will not initialize correctly
  DrumConfigMapper::loadAndApplyDrumKitConfig(drumKit);

  midiTransport.begin();

  setupUsb();

  initWifi();

  network.setup();
  webUI.setup(drumKit);

#if ENABLE_MASS_STORAGE
  enableMassStorageDevice();
#endif

  updateBluetooth();

  // setupTouch();
}

void loop() {
  blinkLed();

  drumKit.updateDrums();

  updateWifiState();

  network.service_traffic();
  updateBluetooth();

  // touchSense();
}

static void logVersion() {
  eventLog.log(Level::Info, String("Version: ") + Version::getPackageVersion()
    + " (Git: " + Version::getGitCommitHash() + ", Date: " + Version::getBuildTime() + ")");
}

static void ledTest() {
  DrumIO::led(LedId::WatchDog, true);
  DrumIO::led(LedId::HitIndicator, true);
  DrumIO::led(LedId::Network, true);
  DrumIO::led(LedId::Ble, true);

  delay(200);

  DrumIO::led(LedId::WatchDog, false);
  DrumIO::led(LedId::HitIndicator, false);
  DrumIO::led(LedId::Network, false);
  DrumIO::led(LedId::Ble, false);
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

static void reconnectUsb() {
#ifdef HAS_TINY_USB
  if (tud_mounted()) {
    tud_disconnect();
    delay(10);
    tud_connect();
  }
#endif
}

static void updateBluetooth() {
#ifdef HAS_BLUETOOTH
  MidiOutputMode mode = drumKit.getMidiOutputMode();
  bleClient.updateClient(mode == MidiOutputMode::BleClient);
  bleServer.updateServer(mode == MidiOutputMode::BleServer);
#endif
}

static void initWifi() {
#ifdef USE_WIFI
  triggerButton.init();

  EDRUM_DEBUGLN("Connecting ...");
  DrumIO::led(LedId::Network, false);
  wifi.connect();
  // wifi.startServer();
  DrumIO::led(LedId::Network, true);
  EDRUM_DEBUGLN("Connected");
#endif
}

static void updateWifiState() {
#ifdef USE_WIFI
  if (triggerButton.isPressed()) {
    DrumIO::led(LedId::Network, false);
    wifi.provision();
    DrumIO::led(LedId::Network, true);
  }
#endif
}
