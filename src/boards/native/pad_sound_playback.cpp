// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pad_sound_playback.h"

#define COUNT 200

struct {
  const uint16_t timeDiffUs[COUNT] = {0, 219, 216, 215, 218, 223, 217, 218, 216, 214, 222, 219, 214, 218, 216, 220, 216, 217, 215, 225, 219, 216, 214, 214, 222, 219, 215, 217, 221, 217, 219, 216, 214, 221, 215, 214, 215, 213, 225, 216, 215, 217, 224, 217, 212, 216, 215, 221, 218, 213, 216, 220, 217, 223, 219, 219, 227, 221, 220, 223, 220, 201, 221, 359, 225, 212, 223, 221, 220, 211, 220, 217, 217, 218, 200, 223, 215, 220, 225, 223, 220, 220, 219, 225, 220, 222, 220, 225, 223, 221, 218, 222, 225, 222, 217, 220, 222, 221, 220, 223, 222, 200, 222, 220, 220, 218, 202, 220, 222, 219, 223, 217, 218, 218, 218, 218, 217, 214, 214, 223, 219, 217, 214, 218, 225, 223, 220, 218, 219, 223, 221, 221, 218, 226, 220, 219, 215, 215, 220, 218, 215, 216, 223, 219, 215, 213, 216, 219, 219, 217, 215, 213, 220, 217, 220, 216, 223, 218, 216, 214, 214, 222, 214, 215, 217, 223, 216, 219, 216, 214, 222, 218, 215, 218, 218, 218, 217, 217, 215, 219, 217, 217, 216, 218, 222, 215, 215, 219, 223, 214, 216, 219, 216, 221, 213, 216, 216, 215, 219, 216};
  const uint8_t values[COUNT] = {2, 2, 0, 4, 0, 0, 4, 0, 0, 4, 0, 2, 0, 0, 2, 4, 4, 2, 4, 4, 0, 2, 4, 2, 2, 4, 4, 2, 4, 2, 4, 4, 4, 2, 0, 4, 2, 2, 0, 2, 0, 2, 2, 2, 0, 0, 4, 4, 2, 4, 0, 10, 2, 54, 104, 138, 164, 128, 120, 98, 92, 74, 82, 64, 80, 88, 90, 72, 50, 44, 36, 4, 2, 34, 40, 24, 22, 72, 74, 46, 30, 54, 78, 88, 70, 76, 80, 82, 74, 54, 32, 24, 24, 42, 56, 52, 34, 30, 34, 42, 44, 38, 36, 34, 40, 38, 40, 30, 18, 18, 16, 16, 18, 14, 14, 14, 10, 2, 4, 0, 2, 2, 6, 16, 20, 28, 22, 20, 26, 30, 32, 30, 30, 30, 22, 18, 16, 10, 8, 0, 6, 10, 12, 12, 14, 12, 8, 0, 6, 14, 14, 10, 8, 12, 6, 2, 2, 0, 4, 6, 4, 8, 4, 10, 8, 6, 0, 2, 2, 2, 6, 4, 4, 6, 6, 6, 8, 2, 2, 8, 6, 4, 0, 2, 2, 0, 2, 4, 2, 0, 4, 4, 2, 2, 2, 4, 2, 8, 14, 16};
  //{6, 6, 6, 2, 4, 6, 2, 4, 4, 2, 6, 4, 4, 4, 4, 4, 2, 6, 2, 2, 6, 6, 2, 2, 2, 4, 2, 6, 2, 6, 2, 4, 4, 4, 4, 2, 4, 6, 4, 2, 6, 4, 6, 4, 4, 6, 2, 2, 4, 4, 4, 2, 4, 4, 4, 6, 6, 6, 4, 6, 4, 2, 4, 6, 2, 2, 4, 2, 4, 2, 4, 4, 4, 2, 6, 2, 2, 2, 2, 4, 4, 4, 4, 4, 0, 4, 0, 2, 4, 0, 2, 2, 4, 2, 4, 4, 2, 6, 2, 0, 4, 0, 2, 2, 4, 0, 4, 4, 2, 2, 2, 4, 4, 4, 4, 2, 2, 2, 6, 4, 6, 2, 4, 2, 6, 2, 4, 2, 4, 6, 4, 6, 4, 2, 4, 4, 2, 4, 2, 4, 6, 4, 2, 2, 4, 4, 4, 2, 4, 2, 4, 2, 4, 2, 6, 6, 2, 2, 4, 4, 2, 4, 2, 4, 4, 4, 2, 4, 2, 4, 2, 2, 2, 2, 4, 6, 2, 4, 6, 4, 2, 4, 6, 6, 4, 2, 4, 4, 2, 2, 4, 4, 2, 4, 6, 2, 6, 2, 2, 4},
} simulationData;

PadSoundPlayback::PadSoundPlayback(const DrumPad* hitPad, bool choke)
  : hitPad(hitPad),
    choke(choke) {
  step = 0;
  nextSignalTimeUs = micros() + simulationData.timeDiffUs[step];
}

void PadSoundPlayback::update() {
  if (step < 0) {
    return;
  } else if (step < COUNT) {
    nextPadValueSteps();
  } else {
    stop();
  }
}

void PadSoundPlayback::nextPadValueSteps() {
  while (micros() >= nextSignalTimeUs) {
    if (choke) {
      nextPadChokeValueStep();
    } else {
      nextPadValueStep();
    }
    ++step;
    nextSignalTimeUs += simulationData.timeDiffUs[step];
  }
}

void PadSoundPlayback::nextPadValueStep() {
  for (zone_size_t zone = 0; zone < hitPad->getActiveZoneCount(); ++zone) {
    float scale = rand() / (double)RAND_MAX;
    sensor_value_t value = scale * simulationData.values[step] / 164 * 1023;
    setPadPinValue(*hitPad, zone, value);
  }
}

void PadSoundPlayback::nextPadChokeValueStep() {
  const zone_size_t chokeZone = 1;
  const sensor_value_t chokeValue = 0.8 * MAX_SENSOR_VALUE;

  for (zone_size_t zone = 0; zone < hitPad->getActiveZoneCount(); ++zone) {
    setPadPinValue(*hitPad, zone, zone == chokeZone ? chokeValue : ZERO_OFFSET);
  }
}

void PadSoundPlayback::stop() {
  for (zone_size_t zone = 0; zone < hitPad->getActiveZoneCount(); ++zone) {
    setPadPinValue(*hitPad, zone, 0);
  }

  SerialDebug.printf("Hit: %s\n", hitPad->getName().c_str());

  step = -1;
  nextSignalTimeUs = 0;
  hitPad = nullptr;
}
