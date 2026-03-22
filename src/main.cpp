// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config/config_mapper.h"
#include "drum_kit.h"
#include "event_log.h"
#include "log.h"
#include "midi_transport.h"
#include "network_connection.h"
#include "usb_device.h"
#include "version.h"
#include "webui.h"

#include <Arduino.h>

DrumKit drumKit;

static void logVersion();

void setup() {  
  DrumIO::setup(true);

#ifdef ENABLE_SERIAL_DEBUG
  SerialDebug.begin(LOG_BAUD);
#endif

  logVersion();

  // Note: config must be loaded before USB is started, otherwise USB will not initialize correctly
  DrumConfigMapper::loadAndApplyDrumKitConfig(drumKit);

  midiTransport.begin();

  UsbDevice::begin();

  networkConnection.begin();

  // TODO: move WebUI server to core1
  webUI.setup(drumKit);

#if ENABLE_MASS_STORAGE
  enableMassStorageDevice();
#endif

  midiTransport.update();

  // setupTouch();
}

void loop() {
  drumKit.updateDrums();

  networkConnection.update();
  midiTransport.update();

  UsbDevice::update();

  DrumIO::resetWatchdog();

  // touchSense();
}

static void logVersion() {
  eventLog.log(Level::Info, String("Version: ") + Version::getPackageVersion()
    + " (Git: " + Version::getGitCommitHash() + ", Date: " + Version::getBuildTime() + ")");
}
