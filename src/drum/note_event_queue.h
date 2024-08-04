// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"

#include <string.h>

struct MidiNoteEvent {
  midi_note_t note;
  time_ms_t noteOnTimeMs;
};

/**
 * Queue to store pending midi note events.
 * The queue is statically allocated to make monitoring of the memory usage easier than
 * the dynamically allocated std::queue.
 */
class NoteEventQueue {
public:
  NoteEventQueue() = default;

  // disable shallow copies
  NoteEventQueue(const NoteEventQueue&) = delete;
  NoteEventQueue& operator=(const NoteEventQueue&) = delete;

  // enable move semantic
  NoteEventQueue(NoteEventQueue&& other) = default;
  NoteEventQueue& operator=(NoteEventQueue&& other) = default;

  void addNote(midi_note_t note, time_ms_t timeMs) {
    if (size < MAX_PENDING_NOTES) {
      pendingNotes[size] = {note, timeMs};
      ++size;
    }
  }

  const MidiNoteEvent& peekOldestNote() const {
    return pendingNotes[0];
  }

  void removeOldestNote() {
    removeIndex(0);
  }

  bool removeNote(midi_note_t note) {
    for (uint8_t i = 0; i < size; ++i) {
      if (pendingNotes[i].note == note) {
        removeIndex(i);
        return true; // every note should be unique in the queue, so do not continue searching
      }
    }
    return false; // note not found
  }

  void removeIndex(uint8_t index) {
    const uint8_t notesAfterIndex = size - index - 1;
    memmove(&pendingNotes[index], &pendingNotes[index + 1], notesAfterIndex * sizeof(MidiNoteEvent));
    --size;
  }

  bool isEmpty() const {
    return size == 0;
  }

  bool isFull() const {
    return size >= MAX_PENDING_NOTES;
  }

  uint8_t getSize() const {
    return size;
  }

public:
  static constexpr size_t MAX_PENDING_NOTES = 32; // each note (~zone of pad) can only be played once at a time, so this should be enough

private:
  MidiNoteEvent pendingNotes[MAX_PENDING_NOTES];
  uint8_t size = 0;
};