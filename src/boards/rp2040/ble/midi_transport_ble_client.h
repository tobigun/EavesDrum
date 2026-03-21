// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport.h"
#include "midi_transport_arduino_midi.h"
#include "midi_serial_ble_client.h"

class MidiTransport_BleClient : public MidiTransport_ArduinoMidi<MidiSerialBleClient> {
public:
  MidiTransport_BleClient(MidiSerialBleClient& serialPort)
    : MidiTransport_ArduinoMidi<MidiSerialBleClient>(serialPort) {}

  virtual void begin();

  virtual void shutdown();

  virtual void update();
};
