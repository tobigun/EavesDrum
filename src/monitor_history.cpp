// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "monitor.h"

#define CONVERT_8BIT(val) ((uint8_t)((val) >> 2))

bool MonitorHistory::addValueEntry(uint16_t timeUntilPreviousUs, const sensor_value_t* sensorValues) {
  // convert values to 8 bit to safe space
  const uint8_t entryValues[3] = {
    CONVERT_8BIT(sensorValues[0]),
    CONVERT_8BIT(sensorValues[1]),
    CONVERT_8BIT(sensorValues[2])};
  return addEntry(timeUntilPreviousUs, false, entryValues);
}

bool MonitorHistory::addGapEntry(uint16_t timeDiffUs) {
  const uint8_t gapValues[3] = {0, 0, 0};
  return addEntry(timeDiffUs, true, gapValues);
}

bool MonitorHistory::addEntry(uint16_t timeUntilPreviousUs, bool isGap, const uint8_t* entryValues) {
  if (isFull()) {
    return false;
  }

  ringBuffer[nextWritePos] = {
      .timeUntilPreviousUs = timeUntilPreviousUs,
      .isGap = isGap,
      .values = {entryValues[0], entryValues[1], entryValues[2]}};

  lastWritePos = nextWritePos;
  nextWritePos = (nextWritePos + 1) % MONITOR_HISTORY_COUNT;
  return true;
}

void MonitorHistory::copyTo(HistoryEntry* historyBuffer) {
  // copy ring buffer from write index to end of buffer, then append older data from start of buffer (index=0) to write index
  // to have a continuous array from start to end
  const int16_t writeIndexToEndCount = MONITOR_HISTORY_COUNT - nextWritePos;
  memcpy(&historyBuffer[0], &ringBuffer[nextWritePos], writeIndexToEndCount * sizeof(HistoryEntry));
  if (nextWritePos > 0) {
    memcpy(&historyBuffer[writeIndexToEndCount], &ringBuffer[0], nextWritePos * sizeof(HistoryEntry));
  }
  historyBuffer[0].timeUntilPreviousUs = 0; // first entry does not have a previous entry
}
