// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Stream.h"
#include "ble_midi_server.h"

class MidiSerialBleServer : public Stream {
public:
  MidiSerialBleServer() {}

  // for MIDI library
  bool begin(uint32_t baud) {
    (void)baud;
    return true;
  }

  // Stream interface to use with MIDI Library
  virtual int read(void) {
    uint8_t ch;
    uint16_t timestamp;
    return ble_midi_server_stream_read(1, &ch, &timestamp) ? (int)ch : (-1);
  }

  virtual size_t write(uint8_t b) { return ble_midi_server_stream_write(1, &b); }
  virtual int available(void) { return true; }
  virtual int peek(void) { return -1; } // MIDI Library does not use peek
  virtual void flush(void) {} // MIDI Library does not use flush

  using Stream::write;
};
