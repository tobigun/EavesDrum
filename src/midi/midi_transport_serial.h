// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "drum_io.h"
#include "midi_transport_arduino_midi.h"
#include "serialMIDI.h"

template<typename SerialType>
class MidiTransport_Serial : public MidiTransport_ArduinoMidi {
public:
  MidiTransport_Serial(SerialType& serial, pin_size_t txPin, bool isSerialPortUsedForLogging = false)
    : MidiTransport_ArduinoMidi(),
      serial(serial),
      txPin(txPin),
      isSerialPortUsedForLogging(isSerialPortUsedForLogging) {}

  void begin() override {
    DrumIO::led(LedId::MidiConnected, true);

#ifdef ENABLE_SERIAL_DEBUG
    if (isSerialPortUsedForLogging) {
      logInfo("Serial Port used by MIDI transport. Logging disabled\n");
      SerialDebug.flush();
      setLogLevel(Level::None);
      SerialDebug.end();
    }
#endif

    serial.setTX(txPin);
    serial.begin(settings.BaudRate);
  }

  void stop() override {
    DrumIO::led(LedId::MidiConnected, false);

#ifdef ENABLE_SERIAL_DEBUG
    if (isSerialPortUsedForLogging) {
      // reset logging
      SerialDebug.begin(LOG_BAUD);
      setLogLevel(DEFAULT_LOG_LEVEL);
      return;
    }
#endif

    serial.end();
  }

  size_t write(uint8_t b) override {
    return serial.write(b);
  }

private:
  SerialType& serial;
  pin_size_t txPin;
  bool isSerialPortUsedForLogging; // set to true if the serial port is shared with logging to disable logging and avoid conflicts
  MIDI_NAMESPACE::DefaultSerialSettings settings;
};
