// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"

struct DrumMappings {
  DrumMappings() = default;
  
  DrumMappings(String role):
    role(role) {}

  String role = nullptr;
  String name = nullptr;

  midi_note_t noteMain = MIDI_NOTE_UNASSIGNED;
  midi_note_t noteRim = MIDI_NOTE_UNASSIGNED;
  midi_note_t noteCup = MIDI_NOTE_UNASSIGNED; // cymbal

  // snare
  midi_note_t noteCross = MIDI_NOTE_UNASSIGNED;

  // hihat
  bool_or_undefined closedNotesEnabled = BOOL_UNDEFINED;
  midi_note_t noteCloseMain = MIDI_NOTE_UNASSIGNED;
  midi_note_t noteCloseRim = MIDI_NOTE_UNASSIGNED;
  midi_note_t noteCloseCup = MIDI_NOTE_UNASSIGNED;
};
