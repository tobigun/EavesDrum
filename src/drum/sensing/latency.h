// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sensing.h"
#include "types.h"
#include "monitor.h"

class DrumPad;

class LatencySensing {
public:
  LatencySensing(DrumPad& pad) : pad(pad) {}

  void sense(time_us_t senseTimeUs, LatencyTestInfo& info);

private:
  void resetState();

private:
  DrumPad& pad;
};
