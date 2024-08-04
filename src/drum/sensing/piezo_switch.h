// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sensing.h"
#include "types.h"

class DrumPad;

class PiezoSwitchSensing {
public:
  PiezoSwitchSensing(DrumPad& pad) : pad(pad) {}

  void sense(time_us_t senseTimeUs);

private:
  void resetHitInfo();
  void readInputValues();

  bool isZoneSwitchPressed(zone_size_t zone, zone_size_t activeZoneCount);

  bool isChoked(zone_size_t activeZoneCount);

  SensingState detectPeak(time_us_t senseTimeUs);

  SensingState scan(time_us_t senseTimeUs);

  inline void updateMaxSensorValue();

  zone_size_t findZoneHitIndex();

private:
  DrumPad& pad;
};
