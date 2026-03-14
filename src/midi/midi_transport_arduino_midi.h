// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_serial_usb_device.h"
#include "midi_transport.h"

#include <MIDI.h>

template <class SerialPort>
class MidiTransport_ArduinoMidi : public MidiTransport {
  using Transport = MIDI_NAMESPACE::SerialMIDI<SerialPort>;

public:
  MidiTransport_ArduinoMidi(SerialPort& serialPort)
    : serialMidi(serialPort),
      midiInterface((Transport&)serialMidi) {}

  virtual void begin() {
    // Initialize MIDI, and listen to all MIDI channels
    // This will also call serial port's begin()
    midiInterface.begin(MIDI_CHANNEL_OMNI);
  }

  virtual void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
    midiInterface.sendNoteOn(inNoteNumber, inVelocity, inChannel);
  }

  virtual void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
    midiInterface.sendNoteOff(inNoteNumber, inVelocity, inChannel);
  }

  virtual void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) {
    midiInterface.sendAfterTouch(inPressure, inChannel);
  }

  virtual void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) {
    midiInterface.sendAfterTouch(inNoteNumber, inPressure, inChannel);
  }

  virtual void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) {
    midiInterface.sendControlChange(inControlNumber, inControlValue, inChannel);
  }

private:
  Transport serialMidi;
  MIDI_NAMESPACE::MidiInterface<Transport> midiInterface;
};
