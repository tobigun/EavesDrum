// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport.h"

#if __has_include(<SPISlave.h>)
#include <SPISlave.h>
#define ENABLE_MIDI_GUITAR_HERO_TRANSPORT

class MidiTransport_GuitarHero : public MidiTransport {
public:
  void start() override;

  void stop() override;

  void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) override;

  void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) override {}

  void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) override {}

  void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) override {}

  void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) override {}
};

#endif
