// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if 0

#include "midi_transport.h"

#include <BLEMIDI_Transport.h>
#include <MIDI.h>
#include <hardware/BLEMIDI_ESP32.h>

class MidiTransport_ArduinoBleMidi : public MidiTransport {
public:
  void begin() {
    //BLEMIDI_INSTANCE.setHandleConnected(onBluetoothConnected);
    //BLEMIDI_INSTANCE.setHandleDisconnected(onBluetoothDisconnected);

    // Initialize MIDI, and listen to all MIDI channels
    // This will also call usb_midi's begin()
    MIDI_INSTANCE.begin(MIDI_CHANNEL_OMNI);
  }

  void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
    MIDI_INSTANCE.sendNoteOn(inNoteNumber, inVelocity, inChannel);
  }

  void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
    MIDI_INSTANCE.sendNoteOff(inNoteNumber, inVelocity, inChannel);
  }

  void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) {
    MIDI_INSTANCE.sendAfterTouch(inPressure, inChannel);
  }

  void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) {
    MIDI_INSTANCE.sendAfterTouch(inNoteNumber, inPressure, inChannel);
  }

  void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) {
    MIDI_INSTANCE.sendControlChange(inControlNumber, inControlValue, inChannel);
  }
};

#endif
