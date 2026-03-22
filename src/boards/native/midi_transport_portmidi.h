// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef ENABLE_MIDI_PORTMIDI_TRANSPORT

#include "midi_transport.h"

class MidiTransport_PortMidi : public MidiTransport {
public:
  void begin() override;

  void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) override;

  void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) override;

  void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) override;

  void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) override;

  void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) override;
};

#endif
