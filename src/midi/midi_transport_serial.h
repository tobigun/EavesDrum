// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "drum_io.h"
#include "midi_transport_arduino_midi.h"
#include "serialMIDI.h"

class MidiTransport_Serial : public MidiTransport_ArduinoMidi {
public:
  MidiTransport_Serial(SerialUART& serial)
    : MidiTransport_ArduinoMidi(),
      serial(serial) {}

  void begin() override {
    DrumIO::led(LedId::MidiConnected, true);

#ifdef ENABLE_SERIAL_DEBUG
    if (isLogSerial()) {
      logInfo("Serial Port used by MIDI transport. Logging disabled\n");
      serial.flush();
      setLogLevel(Level::None);
    }
#endif

    serial.begin(settings.BaudRate);
  }

  void stop() override {
    DrumIO::led(LedId::MidiConnected, false);

#ifdef ENABLE_SERIAL_DEBUG
    if (isLogSerial()) {
      // reset logging
      serial.begin(LOG_BAUD);
      setLogLevel(DEFAULT_LOG_LEVEL);
      return;
    }
#endif

    serial.end();
  }

  size_t write(uint8_t b) override {
    return serial.write(b);
  }

  bool isLogSerial() {
    return &serial == &SerialDebug;
  }

private:
  SerialUART& serial;
  MIDI_NAMESPACE::DefaultSerialSettings settings;
};
