// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport.h"
#include "midi_transport_arduino_midi.h"
#include "midi_serial_ble_client.h"

class MidiTransport_BleClient : public MidiTransport_ArduinoMidi<MidiSerialBleClient> {
public:
  MidiTransport_BleClient()
    : MidiTransport_ArduinoMidi<MidiSerialBleClient>(midiSerialBleClient) {}

  void begin() override;
  void shutdown() override;
  void update() override;

private:
  MidiSerialBleClient midiSerialBleClient;
};
