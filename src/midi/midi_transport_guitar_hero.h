// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport.h"

class MidiTransport_GuitarHero : public MidiTransport {
public:
  virtual void begin();

  virtual void shutdown();

  virtual void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel);

  virtual void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {}

  virtual void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) {}

  virtual void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) {}

  virtual void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) {}
};
