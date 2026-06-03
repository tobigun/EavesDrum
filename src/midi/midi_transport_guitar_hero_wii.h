// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if __has_include(<wiimote.h>)
#define ENABLE_MIDI_GUITAR_HERO_WII_TRANSPORT

#include "midi_transport.h"

class MidiTransport_GuitarHero_Wii : public MidiTransport {
public:
  void start(MidiOutputMode mode) override;

  void stop() override;

  void update() override;

  void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) override;

  void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) override {}

  void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) override {}

  void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) override {}

  void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) override;
};

#endif
