// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport_arduino_midi.h"
#include "ble_midi_server.h"

class MidiTransport_BleServer : public MidiTransport_ArduinoMidi {
public:
  void begin() override;
  void stop() override;
  void update() override;

  size_t write(uint8_t b) override {
    return ble_midi_server_stream_write(1, &b);
  }
};
