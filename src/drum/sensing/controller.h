// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sensing.h"
#include "types.h"

class DrumPad;

class ControllerSensing {
public:
  ControllerSensing(DrumPad& pad) : pad(pad) {}

  void sense(time_us_t senseTimeUs);

private:
  void hiHatPedalPositionAndChickSensing(time_us_t senseTimeUs, sensor_value_t pedalValue);
  void hiHatPedalCCSensing(sensor_value_t pedalValue);

  static sensor_value_t reduceNoise(sensor_value_t newValue, sensor_value_t oldValue, uint8_t moveDetectTolerance);

private:
  DrumPad& pad;
};
