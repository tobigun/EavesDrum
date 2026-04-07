// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ble_midi_server.h"
#include "midi_transport_ble_server.h"
#include "ble_server_midi_profile.h"
#include "log.h"
#include "drum_io.h"

#define SCAN_RESP_DATA_SIZE 20
static const uint8_t scan_resp_data[SCAN_RESP_DATA_SIZE] = {
  // Name
  SCAN_RESP_DATA_SIZE-1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'E', 'a', 'v', 'e', 's', 'D', 'r', 'u', 'm', ' ', 'B', 'L', 'E', ' ', 'M', 'I', 'D', 'I'
};

bool isConnected = false;

static void updateConnectionStatus();

static void startServer() {
  logInfo("BLE server started\n");
  ble_midi_server_init(profile_data, scan_resp_data, sizeof(scan_resp_data),
      IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
      SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
}

static void stopServer() {
  logInfo("BLE server stopping ...");
  ble_midi_server_deinit();
  updateConnectionStatus();
  logInfo("BLE server stopped");
}

static void updateConnectionStatus() {
  bool wasConnected = isConnected;
  isConnected = ble_midi_server_is_connected();

  if (wasConnected != isConnected) {
    logInfo("BLE server state changed: %s\n", isConnected ? "connected" : "disconnected");
    DrumIO::led(LedId::MidiConnected, isConnected);
    return;
  }

  static bool blinkState = false;
  if (ble_midi_server_is_initialized() && !isConnected) {
    // waiting for connection, blink the LED
    DrumIO::led(LedId::MidiConnected, blinkState);
    blinkState = !blinkState;
  }
}

void MidiTransport_BleServer::begin() {
  startServer();
}

void MidiTransport_BleServer::stop() {
  stopServer();
}

void MidiTransport_BleServer::update() {
  uint32_t currentTimeMs = millis();

  static uint32_t lastSyncTimeMs = 0;
  if (currentTimeMs - lastSyncTimeMs > 1000) {
    lastSyncTimeMs = currentTimeMs;

    updateConnectionStatus();
  }
}
