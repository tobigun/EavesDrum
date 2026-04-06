// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sensing.h"
#include "types.h"

enum class SensingState {
  PeakDetect,
  Scan,
  Mask
};

class DrumPad;

class Sensing {
protected:
  Sensing(DrumPad& pad) : pad(pad) { }

  void resetHitInfo();
  void readInputValues();

  bool isChoked(zone_size_t activeZoneCount);
  SensingState handleChoked();

  bool isZoneSwitchPressed(zone_size_t zone, zone_size_t activeZoneCount);

  void logHit(zone_size_t hitIndex, zone_size_t zoneCount);

  static zone_size_t findMaxValueIndex(sensor_value_t* values, zone_size_t zoneCount, zone_size_t preferedIndex);

protected:
  DrumPad& pad;
};

class PiezoSensing : public Sensing {
public:
  PiezoSensing(DrumPad& pad) : Sensing(pad) { }

  void sense(time_us_t senseTimeUs);

private:
  SensingState detectPeak(time_us_t senseTimeUs);
  SensingState scan(time_us_t senseTimeUs);

  static zone_size_t determineHitZone(sensor_value_t* evaluationVelocities, zone_size_t zoneCount, int8_t headRimBias);
  
  inline void updateMaxSensorValue(zone_size_t activeZoneCount);
};

class PiezoSwitchSensing : public Sensing {
public:
  PiezoSwitchSensing(DrumPad& pad) : Sensing(pad) {}

  void sense(time_us_t senseTimeUs);

private:
  SensingState detectPeak(time_us_t senseTimeUs);
  SensingState scan(time_us_t senseTimeUs);

  zone_size_t determineHitZone();

  inline void updateMaxSensorValue();
};
