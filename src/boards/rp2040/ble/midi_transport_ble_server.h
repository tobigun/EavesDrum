// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport.h"
#include "midi_transport_arduino_midi.h"
#include "midi_serial_ble_server.h"

class MidiTransport_BleServer : public MidiTransport_ArduinoMidi<MidiSerialBleServer> {
public:
  MidiTransport_BleServer()
    : MidiTransport_ArduinoMidi<MidiSerialBleServer>(midiSerialBleServer) {}

  virtual void begin();

  virtual void shutdown();

  virtual void update();

private:
  MidiSerialBleServer midiSerialBleServer;
};
