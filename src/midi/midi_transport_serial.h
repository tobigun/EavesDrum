// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "drum_io.h"
#include "midi_transport_arduino_midi.h"

class MidiTransport_Serial : public MidiTransport_ArduinoMidi {
public:
  MidiTransport_Serial(SerialUART& serial)
    : MidiTransport_ArduinoMidi(),
      serial(serial) {}

  void begin() override {
    if (isLogSerial()) {
      logInfo("Serial Port used by MIDI transport. Logging disabled\n");
      serial.flush();
      setLogLevel(Level::None);
    }
    DrumIO::led(LedId::MidiConnected, true);

    MidiTransport_ArduinoMidi::begin();
  }

  void shutdown() override {
    MidiTransport_ArduinoMidi::shutdown();

    if (isLogSerial()) {
      SerialDebug.begin(LOG_BAUD);
      setLogLevel(DEFAULT_LOG_LEVEL);
    }
    DrumIO::led(LedId::MidiConnected, false);
  }

  size_t write(uint8_t b) override {
    return serial.write(b);
  }

  bool isLogSerial() {
    return &serial == &SerialDebug;
  }

private:
  SerialUART& serial;
};
