// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_pad.h"
#include "drum_log.h"

#include "sensing/piezo.h"
#include "sensing/scale.h"

const uint32_t MAX_PREFERENCE_MULT = 5;


void PiezoSensing::sense(time_us_t senseTimeUs) {
  if (!pad.isConnectorActive()) {
    return;
  }

  const zone_size_t zoneCount = pad.getActiveZoneCount();
  for (zone_size_t zone = 0; zone < zoneCount; ++zone) {
    pad.sensorValues[zone] = pad.readInput(pad.connector->getPin(zone), pad.autoCalibrate);
    pad.hits[zone] = false;
    pad.hitVelocities[zone] = 0;
  }

  if (pad.sensingState == SensingState::PeakDetect) { // check for first peak
    pad.sensingState = peakDetect(senseTimeUs);
  } else if (pad.sensingState == SensingState::Scan) { // search highest peak
    pad.sensingState = scan(senseTimeUs);
  } else { // SensingState::Mask
    bool maskActive = senseTimeUs - pad.scanTimeEndUs < pad.settings.maskTimeMs * 1000;
    if (!maskActive) {
      pad.sensingState = SensingState::PeakDetect;
    }
  }
}

SensingState PiezoSensing::peakDetect(time_us_t senseTimeUs) {
  const zone_size_t zoneCount = pad.getActiveZoneCount();

  bool isPeakDetected = false;
  for (zone_size_t zone = 0; zone < zoneCount; ++zone) {
    bool isPeak = pad.sensorValues[zone] >= pad.settings.zoneThresholdsMin[zone];
    isPeakDetected |= isPeak;
  }

  if (!isPeakDetected) {
    return SensingState::PeakDetect;
  }

  // scan time start
  pad.hitTimeUs = senseTimeUs;

  for (zone_size_t zone = 0; zone < zoneCount; ++zone) {
    pad.maxZoneValues[zone] = pad.sensorValues[zone]; // first peak
  }

  return SensingState::Scan;
}

zone_size_t PiezoSensing::findMaxValueIndex(uint16_t* values, zone_size_t zoneCount, zone_size_t preferedIndex) {
  uint16_t maxValue = values[preferedIndex];
  zone_size_t maxIndex = preferedIndex;
  for (zone_size_t index = 0; index < zoneCount; ++index) {
    if (values[index] > maxValue) {
      maxValue = values[index];
      maxIndex = index;
    }
  }
  return maxIndex;
}

zone_size_t PiezoSensing::determineHitZone(sensor_value_t* evaluationVelocities, zone_size_t zoneCount, int8_t headRimBias) {
  if (zoneCount <= 1) {
    return 0;
  }

  zone_size_t preferedIndex;
  if (headRimBias < 0) { // prefer head
    preferedIndex = 0;
    evaluationVelocities[0] += (uint32_t) evaluationVelocities[0] * MAX_PREFERENCE_MULT * -headRimBias / 100;
  } else { // >= 0 -> prefer rim
    preferedIndex = 1;
    if (headRimBias > 0) {
      evaluationVelocities[1] += (uint32_t) evaluationVelocities[1] * MAX_PREFERENCE_MULT * headRimBias / 100;
    }
  }
  return findMaxValueIndex(evaluationVelocities, zoneCount, preferedIndex);
}

SensingState PiezoSensing::scan(time_us_t senseTimeUs) {
  const zone_size_t zoneCount = pad.getActiveZoneCount();
  for (zone_size_t zone = 0; zone < zoneCount; ++zone) {
    if (pad.sensorValues[zone] > pad.maxZoneValues[zone]) {
      pad.maxZoneValues[zone] = pad.sensorValues[zone];
    }
  }

  bool scanInProgress = senseTimeUs - pad.hitTimeUs < pad.settings.scanTimeUs;
  if (scanInProgress) {
    return SensingState::Scan;
  }

  // scan time end -> start of mask time
  pad.scanTimeEndUs = senseTimeUs;

  sensor_value_t evaluationVelocities[3];
  for (zone_size_t zone = 0; zone < zoneCount; ++zone) {
    sensor_value_t scaledValue = scale(pad.maxZoneValues[zone],
      pad.settings.zoneThresholdsMin[zone],
      pad.settings.zoneThresholdsMax[zone]);
    evaluationVelocities[zone] = scaledValue;
    pad.hitVelocities[zone] = curve(scaledValue, pad.settings.curveType);
  }

  // TODO: handle crossNoteEnabled

  zone_size_t hitIndex = determineHitZone(evaluationVelocities, zoneCount, pad.settings.headRimBias);
  pad.hits[hitIndex] = true;

  DEBUG_PRINTF("[Hit '%s' %s] head: %d/127 (%d/" MAX_SENSOR_VALUE_STR ") rim: %d/127 (%d/" MAX_SENSOR_VALUE_STR ")\n",
      name.c_str(),
      hitIndex == 0 ? "Head" : "Rim",
      hitVelocities[0], maxZoneValues[0],
      (zoneCount >= 2 ? hitVelocities[1] : 0), (zoneCount >= 2 ? maxZoneValues[1] : 0));

  return SensingState::Mask;
}
