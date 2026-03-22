// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport.h"

#include <MIDI.h>

//  using Transport = MIDI_NAMESPACE::SerialMIDI<SerialPort>;

class MidiTransport_ArduinoMidi : public MidiTransport {
public:
  MidiTransport_ArduinoMidi()
    : midiInterface(*this) {}

public: // Transport implementation for MidiInterface
  static const bool thruActivated = false;

  virtual bool beginTransmission(MIDI_NAMESPACE::MidiType midiType) { return true; }

  virtual void endTransmission() {}

  virtual size_t write(uint8_t b) = 0;

  int read(void) { return -1; } // read not used by EavesDrum

  unsigned available() { return 0; } // read not used by EavesDrum

public: // MidiTransport implementation
  void begin() override {
    // Initialize MIDI, and listen to all MIDI channels
    // This will also call serial port's begin()
    midiInterface.begin(MIDI_CHANNEL_OMNI);
  }

  void shutdown() override {}

  void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) override {
    midiInterface.sendNoteOn(inNoteNumber, inVelocity, inChannel);
  }

  void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) override {
    midiInterface.sendNoteOff(inNoteNumber, inVelocity, inChannel);
  }

  void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) override {
    midiInterface.sendAfterTouch(inPressure, inChannel);
  }

  void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) override {
    midiInterface.sendAfterTouch(inNoteNumber, inPressure, inChannel);
  }

  void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) override {
    midiInterface.sendControlChange(inControlNumber, inControlValue, inChannel);
  }

private:
  MIDI_NAMESPACE::MidiInterface<MidiTransport_ArduinoMidi> midiInterface;
};
