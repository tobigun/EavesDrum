// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>

#define NUM_PADS 6

enum GHDrumPadId {
  GH_Red,
  GH_Blue,
  GH_Green,
  GH_Yellow,
  GH_Orange,
  GH_Kick
};

#define GH_RED_NOTE_DEFAULT 38 // Acoustic Snare
#define GH_BLUE_NOTE_DEFAULT 48 // Hi-Mid Tom
#define GH_GREEN_NOTE_DEFAULT 45 // Low Tom
#define GH_YELLOW_NOTE_DEFAULT 46 // Open Hi-Hat
#define GH_ORANGE_NOTE_DEFAULT 49 // Crash Cymbal
#define GH_KICK_NOTE_DEFAULT 36 // Bass Drum 1

#define UNIFIED_BLUE_NOTE 47
#define UNIFIED_GREEN_NOTE 41
#define UNIFIED_YELLOW_NOTE 50
#define UNIFIED_ORANGE_NOTE 51

inline int8_t padIndexToNote(uint8_t padIndex) {
  const uint8_t indexToNote[NUM_PADS] = {
      GH_RED_NOTE_DEFAULT,
      GH_BLUE_NOTE_DEFAULT,
      GH_GREEN_NOTE_DEFAULT,
      GH_YELLOW_NOTE_DEFAULT,
      GH_ORANGE_NOTE_DEFAULT,
      GH_KICK_NOTE_DEFAULT,
  };
  return indexToNote[padIndex];
}

// General Midi Percussion note names:
// https://musescore.org/sites/musescore.org/files/General%20MIDI%20Standard%20Percussion%20Set%20Key%20Map.pdf
inline int8_t noteToPadId(uint8_t noteNumber) {
  switch (noteNumber) {
  case GH_RED_NOTE_DEFAULT: // GH / RB: D1 (Acoustic Snare)
  case 31: // RB: G0 (n.a.)
  case 34: // RB: A#0 (n.a.)
  case 37: // RB: C#1 (Side Stick)
  case 39: // RB: D#1 (Hand Clap)
  case 40: // RB: E1 (Electric Snare)
    return GH_Red;
  case GH_BLUE_NOTE_DEFAULT: // GH: C2 (Hi-Mid Tom)
  case UNIFIED_BLUE_NOTE: // RB: B1 (Low-Mid Tom)
    return GH_Blue;
  case GH_GREEN_NOTE_DEFAULT: // GH: A1 (Low Tom)
  case UNIFIED_GREEN_NOTE: // RB: F1 (Low Floor Tom)
  case 43: // RB: G1 (High Floor Tom)
    return GH_Green;
  case GH_YELLOW_NOTE_DEFAULT: // GH / RB: A#1 (Open Hi-Hat)
  case UNIFIED_YELLOW_NOTE: // RB: D2 (High Tom)
  case 22: // RB Cymbal(Y): A#-1 (n.a.)
  case 26: // RB Cymbal(Y): D0 (n.a.)
  case 42: // RB Cymbal(Y): F#1 (Closed Hi-Hat)
  case 54: // RB Cymbal(Y): F#2 (Tambourine)
  case 44: // RB HiHat Pedal: G#1 (Pedal Hi-Hat)
    return GH_Yellow;
  case GH_ORANGE_NOTE_DEFAULT: // GH / RB Cymbal(G): C#2 (Crash Cymbal 1)
  case 52: // RB Cymbal(G): E2 (Chinese Cymbal)
  case 55: // RB Cymbal(G): G2 (Splash Cymbal)
  case 57: // RB Cymbal(G): A2 (Crash Cymbal 2)
  case UNIFIED_ORANGE_NOTE: // RB Cymbal(B): D#2 (Ride Cymbal 1)
  case 53: // RB Cymbal(B): F2 (Ride Bell)
  case 56: // RB Cymbal(B): G#2 (Cowbell)
  case 59: // RB Cymbal(B): B2 (Ride Cymbal 2)
    return GH_Orange;
  case GH_KICK_NOTE_DEFAULT: // GH / RB: C1 (Bass Drum 1)
  case 33: // RB: A0 (n.a.)
  case 35: // RB: B0 (Acoustic Bass Drum)
    return GH_Kick;
  default:
    return -1;
  }
}
