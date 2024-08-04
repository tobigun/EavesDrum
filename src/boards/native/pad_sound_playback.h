// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "simulation.h"

class PadSoundPlayback {
public:
  PadSoundPlayback(const DrumPad* hitPad, bool choke);

  void update();

  bool isPlaying() {
    return step != -1;
  }

private:
  void nextPadValueSteps();
  void nextPadValueStep();
  void nextPadChokeValueStep();
  void stop();

private:
  const DrumPad* hitPad = nullptr;
  bool choke;
  int step = -1;
  time_us_t nextSignalTimeUs = 0;
};
