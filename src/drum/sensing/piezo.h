// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sensing.h"
#include "types.h"

class DrumPad;

class PiezoSensing {
public:
  PiezoSensing(DrumPad& pad) : pad(pad) {}

  void sense(time_us_t senseTimeUs);

private:
    SensingState peakDetect(time_us_t senseTimeUs);
    SensingState scan(time_us_t senseTimeUs);
    
    static zone_size_t findMaxValueIndex(uint16_t* values, zone_size_t zoneCount, zone_size_t preferedIndex);
    static zone_size_t determineHitZone(sensor_value_t* evaluationVelocities, zone_size_t zoneCount, int8_t headRimBias);
    
    void resetHitInfo();
    void readInputValues();
    
    bool isZoneSwitchPressed(zone_size_t zone, zone_size_t activeZoneCount);
    bool isChoked(zone_size_t activeZoneCount);
    
private:
  DrumPad& pad;
};
