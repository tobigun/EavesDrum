// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ble_server.h"

#include "ble_midi_server.h"
#include "ble/ble_midi_profile.h"
#include "log.h"

BleServer bleServer;

static const uint8_t scan_resp_data[] = {
  // Name
  //0x13, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'E', 'a', 'v', 'e', 's', 'D', 'r', 'u', 'm', ' ', 'B', 'L', 'E', ' ', 'M', 'I', 'D', 'I'
  0x11, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'B', 'L', 'E', '-', 'M', 'I', 'D', 'I', '2', 'U', 'S', 'B', 'H','U','B'
};

static void startServer() {
  ble_midi_server_init(profile_data, scan_resp_data, sizeof(scan_resp_data),
      IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
      SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
}

static void stopServer() {
  ble_midi_server_deinit();
}

void BleServer::updateServer(bool enabled) {
  if (ble_midi_server_is_connected()) {
    uint16_t timestamp;
    uint8_t mes[3];
    uint8_t nread = ble_midi_server_stream_read(sizeof(mes), mes, &timestamp);
    if (nread != 0) {
      SerialDebug.print("Received MIDI from Bluetooth: ");
      // Ignore timestamps for now. Handling timestamps has a few issues:
      // 1. Some applications (e.g., TouchDAW 2.3.1 for Android or Midi Wrench on an iPad)
      //    always send timestamp value of 0.
      // 2. Synchronizing the timestamps to the system clock has issues if there are
      //    lost or out of order packets.
      //uint32_t nwritten = tuh_midi_stream_write(midi_dev_addr, 0, mes, nread);
    }
  }
}
