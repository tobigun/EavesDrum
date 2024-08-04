// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"

enum DrumMappingId {
  NOTE_MAIN,
  NOTE_RIM,
  NOTE_CUP,
  NOTE_CROSS,

  ENABLE_CLOSED_NOTES,
  NOTE_CLOSE_MAIN,
  NOTE_CLOSE_RIM,
  NOTE_CLOSE_CUP,

  ENABLE_PEDAL_CHICK
};

struct DrumMappings {
  midi_note_t noteMain = 0;
  midi_note_t noteRim = 0;
  midi_note_t noteCup = 0; // cymbal

  // snare
  midi_note_t noteCross = 0;

  // hihat
  bool closedNotesEnabled = false;
  midi_note_t noteCloseMain = 0;
  midi_note_t noteCloseRim = 0;
  midi_note_t noteCloseCup = 0;

  bool pedalChickEnabled = false;
};
