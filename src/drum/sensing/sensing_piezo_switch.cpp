// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"
#include "drum_pad.h"

#include "sensing/scale.h"
#include "sensing/sensing.h"

static const size_t MAIN_PIEZO_INDEX = 0;
static const size_t SWITCH_ZONE_OFFSET = 1;

void PiezoSwitchSensing::sense(time_us_t senseTimeUs) {
  if (!pad.isConnectorActive()) {
    return;
  }

  resetHitInfo();
  readInputValues();

  if (pad.sensingState == SensingState::PeakDetect) { // search for first peak
    pad.sensingState = detectPeak(senseTimeUs);
  } else if (pad.sensingState == SensingState::Scan) { // search highest peak
    pad.sensingState = scan(senseTimeUs);
  } else if (pad.sensingState == SensingState::Mask) { // mask: ignore hits
    bool maskActive = senseTimeUs - pad.scanTimeEndUs < pad.settings.maskTimeMs * 1000;
    if (!maskActive) {
      pad.sensingState = SensingState::PeakDetect;
    }
  }
}

SensingState PiezoSwitchSensing::detectPeak(time_us_t senseTimeUs) {
  const zone_size_t activeZoneCount = pad.getActiveZoneCount();

  bool isPeakDetected = pad.sensorValues[MAIN_PIEZO_INDEX] >= pad.settings.zoneThresholdsMin[MAIN_PIEZO_INDEX];

  bool isMaybeChoked = isChoked(activeZoneCount); // we cannot be sure the cymbal is choked as a hit might still be detected

  if (!isPeakDetected && !isMaybeChoked) { // no hit or choke detected -> keep current state
    return SensingState::PeakDetect;
  }

  // peak detected or cymbal might be choked -> start scan time
  pad.hitTimeUs = senseTimeUs;

  for (zone_size_t zone = 0; zone < activeZoneCount; ++zone) {
    pad.maxZoneValues[zone] = 0;
  }
  updateMaxSensorValue();

  return SensingState::Scan;
}

SensingState PiezoSwitchSensing::scan(time_us_t senseTimeUs) {
  updateMaxSensorValue();

  bool scanInProgress = senseTimeUs - pad.hitTimeUs < pad.settings.scanTimeUs;
  if (scanInProgress) {
    return SensingState::Scan;
  }

  // scan time end -> start of mask time
  pad.scanTimeEndUs = senseTimeUs;

  sensor_value_t piezoThresholdMin = pad.settings.zoneThresholdsMin[MAIN_PIEZO_INDEX];
  sensor_value_t piezoThresholdMax = pad.settings.zoneThresholdsMax[MAIN_PIEZO_INDEX];
  sensor_value_t maxPiezoValue = pad.maxZoneValues[MAIN_PIEZO_INDEX];

  if (maxPiezoValue == 0) { // choked
    return handleChoked();
  }
  
  zone_size_t hitIndex = determineHitZone();
  pad.hitVelocities[hitIndex] = scaleAndCurve(maxPiezoValue, piezoThresholdMin, piezoThresholdMax, pad.settings.curveType);
  pad.hits[hitIndex] = true;
  pad.cymbal.lastEventType = LastCymbalEventType::Hit;
  logHit(hitIndex, pad.getActiveZoneCount());

  return SensingState::Mask;
}

inline void PiezoSwitchSensing::updateMaxSensorValue() {
  // update max piezo value
  const sensor_value_t piezoValue = pad.sensorValues[MAIN_PIEZO_INDEX];
  if (piezoValue > pad.maxZoneValues[MAIN_PIEZO_INDEX]
      && piezoValue >= pad.settings.zoneThresholdsMin[MAIN_PIEZO_INDEX]) { // Note: threshold must be checked as pad could be choked, i.e. old value could be 0
    pad.maxZoneValues[MAIN_PIEZO_INDEX] = piezoValue;
  }

  // update switches (edge, cup, ...)
  const zone_size_t activeZoneCount = pad.getActiveZoneCount();
  for (zone_size_t zone = SWITCH_ZONE_OFFSET; zone < activeZoneCount; ++zone) {
    if (isZoneSwitchPressed(zone, activeZoneCount)) {
      ++pad.maxZoneValues[zone];
    }
  }
}

zone_size_t PiezoSwitchSensing::determineHitZone() {
  const zone_size_t zoneCount = pad.getActiveZoneCount();
  if (zoneCount <= SWITCH_ZONE_OFFSET) {
    return MAIN_PIEZO_INDEX; // cymbal has no switches, must be main zone
  }
  zone_size_t switchCount = zoneCount - SWITCH_ZONE_OFFSET;

  sensor_value_t* switchPressCounters = &pad.maxZoneValues[SWITCH_ZONE_OFFSET];
  zone_size_t switchHitIndex = findMaxValueIndex(switchPressCounters, switchCount, 0);
  return (switchPressCounters[switchHitIndex] > 0)
    ? switchHitIndex + SWITCH_ZONE_OFFSET
    : MAIN_PIEZO_INDEX;
}
