// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>

#define HISTORY_INDEX_NONE -1
#define MONITOR_HISTORY_COUNT 200 // 50

typedef uint8_t history_bool_t; // platform independant replacement for "bool"
typedef int16_t history_index_t;

struct HistoryEntry {
  uint16_t timeUntilPreviousUs;
  history_bool_t isGap = false;
  uint8_t values[3] = {0, 0, 0}; // 0..255
} __attribute__((packed));

class MonitorHistory {
public:
  // returns the buffer position the value was written to
  bool addValueEntry(uint16_t timeUntilPreviousUs, const sensor_value_t* sensorValues);
  bool addGapEntry(uint16_t timeUntilPreviousUs);

  /**
   * Returns true if the buffer is full, i.e. the next write would overwrite the start of the trigger event.
   */
  bool isFull() const {
    return triggerStartPos != HISTORY_INDEX_NONE && triggerStartPos == nextWritePos;
  }

  /**
   * Copies the ring buffer contents to a normal buffer, starting with the oldest entry (i.e. the next write position).
   */
  void copyTo(HistoryEntry* historyBuffer);

  /**
   * Returns the trigger start position relative to the next write position (i.e. the start of the buffer).
   */
  history_index_t getRelativeTriggerStartPos() const {
    return getRelativePos(triggerStartPos);
  }
  
  /**
   * Whether the start of a trigger event was marked.
   * If this returns true the trigger entries are preotected against overwriting (see {@link isFull()}).
   */
  bool isTriggerStartPosSet() const {
    return triggerStartPos != HISTORY_INDEX_NONE;
  }

  /**
   * Sets the start (and end) of the trigger which marks the start of an event (start of hit scan time, pedal move, ...).
   * If the start of a trigger is set, this entry will not be overwritten, i.e. it marks the end of the ring buffer.
   * See {@link isFull()}.
   */
  void setTriggerStartPos() {
    if (lastWritePos != HISTORY_INDEX_NONE) {
        triggerStartPos = lastWritePos;
        triggerEndPos = lastWritePos;
    }
  }

  /**
   * Returns the trigger end position relative to the next write position (i.e. the start of the buffer).
   */
  history_index_t getRelativeTriggerEndPos() const {
    return getRelativePos(triggerEndPos);
  }

  bool isTriggerEndPosSet() const {
    return triggerEndPos != HISTORY_INDEX_NONE;
  }

  /**
   * Sets the end of the trigger (i.e. like the end of the hit scan time).
   * The area between start and end of a trigger is usually highlighted in the UI chart.
   */
  void setTriggerEndPos() {
    if (lastWritePos != HISTORY_INDEX_NONE) {
        triggerEndPos = lastWritePos;
    }
  }

  /**
   * Resets the trigger start and end positions and removes the protection of the trigger entries (see {@link isFull()}).
   */
  void resetTrigger() {
    triggerStartPos = HISTORY_INDEX_NONE;
    triggerEndPos = HISTORY_INDEX_NONE;
  }

private:
  history_index_t lastWritePos = HISTORY_INDEX_NONE;
  history_index_t nextWritePos = 0;
  history_index_t triggerStartPos = HISTORY_INDEX_NONE;
  history_index_t triggerEndPos = HISTORY_INDEX_NONE;

  HistoryEntry ringBuffer[MONITOR_HISTORY_COUNT];

private:
  bool addEntry(uint16_t timeUntilPreviousUs, bool isGap, const uint8_t* values);

    // adjust indizes as they are now relative to offset 0
  history_index_t getRelativePos(history_index_t index) const {
    const history_index_t writePosToEndCount = MONITOR_HISTORY_COUNT - nextWritePos;
    return (index + writePosToEndCount) % MONITOR_HISTORY_COUNT;
  }
};
