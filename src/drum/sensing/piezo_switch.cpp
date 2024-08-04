// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_log.h"
#include "drum_pad.h"

#include "sensing/scale.h"
#include "sensing/piezo_switch.h"

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
  } else { // mask: ignore hits
    bool maskActive = senseTimeUs - pad.scanTimeEndUs < pad.settings.maskTimeMs * 1000;
    if (!maskActive) {
      pad.sensingState = SensingState::PeakDetect;
    }
  }
}

void PiezoSwitchSensing::resetHitInfo() {
  const zone_size_t zoneCount = pad.getActiveZoneCount();
  for (zone_size_t zone = 0; zone < zoneCount; ++zone) {
    pad.hits[zone] = false;
    pad.hitVelocities[zone] = 0;
  }
  pad.cymbal.isChoked = false;
}

void PiezoSwitchSensing::readInputValues() {
  const zone_size_t pinCount = pad.getActivePinCount();
  for (zone_size_t pinIndex = 0; pinIndex < pinCount; ++pinIndex) {
    sensor_value_t inputValue = pad.readInput(pad.connector->getPin(pinIndex));

    if (pad.settings.zonesType == ZonesType::Zones3_PiezoAndSwitches_1TRS && pinIndex == 1) {
      uint8_t sensorIndex = (inputValue >= pad.settings.zoneThresholdsMin[2]) ? 2 : 1;
      pad.sensorValues[sensorIndex] = inputValue;
      pad.sensorValues[3 - sensorIndex] = 0; // set the other zone to 0
    } else {
      pad.sensorValues[pinIndex] = inputValue;
    }
  }
}

bool PiezoSwitchSensing::isZoneSwitchPressed(zone_size_t zone, zone_size_t activeZoneCount) {
  return pad.sensorValues[zone] >= pad.settings.zoneThresholdsMin[zone];
}

bool PiezoSwitchSensing::isChoked(zone_size_t activeZoneCount) {
  if (pad.settings.chokeType == ChokeType::None) {
    return false;
  }

  zone_size_t chokeSwitchZoneIndex = (pad.settings.chokeType == ChokeType::Switch_Edge) ? 1 : 2;
  bool hasActiveChokeSwitch = activeZoneCount > chokeSwitchZoneIndex;
  if (!hasActiveChokeSwitch) {
    return false;
  }

  return isZoneSwitchPressed(chokeSwitchZoneIndex, activeZoneCount);
}

SensingState PiezoSwitchSensing::detectPeak(time_us_t senseTimeUs) {
  bool isPeakDetected = pad.sensorValues[MAIN_PIEZO_INDEX] >= pad.settings.zoneThresholdsMin[MAIN_PIEZO_INDEX];

  const zone_size_t activeZoneCount = pad.getActiveZoneCount();
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

  bool scanActive = senseTimeUs - pad.hitTimeUs < pad.settings.scanTimeUs;
  if (scanActive) {
    return SensingState::Scan;
  }

  // scan time end -> start of mask time
  pad.scanTimeEndUs = senseTimeUs;

  sensor_value_t piezoThresholdMin = pad.settings.zoneThresholdsMin[MAIN_PIEZO_INDEX];
  sensor_value_t piezoThresholdMax = pad.settings.zoneThresholdsMax[MAIN_PIEZO_INDEX];
  sensor_value_t maxPiezoValue = pad.maxZoneValues[MAIN_PIEZO_INDEX];

  zone_size_t hitIndex = findZoneHitIndex();
  if (maxPiezoValue == 0) {
    if (pad.cymbal.lastEventType != LastCymbalEventType::Choked) {
      pad.cymbal.isChoked = true;
      pad.cymbal.lastEventType = LastCymbalEventType::Choked;
      DEBUG_PRINTF("[Choked '%s'] \n", name.c_str());
    } else {
      // do not choke again if already choked.
      // This will enable us to detect choked hits that would otherwise be masked by the choke events
      return SensingState::PeakDetect;
    }
  } else {
    pad.hitVelocities[hitIndex] = scaleAndCurve(maxPiezoValue, piezoThresholdMin, piezoThresholdMax, pad.settings.curveType);
    pad.hits[hitIndex] = true;
    pad.cymbal.lastEventType = LastCymbalEventType::Hit;
    DEBUG_PRINTF("[Hit '%s' %s] bow: %d/127 (%d/" MAX_SENSOR_VALUE_STR ") edge: %d/127 (%d/" MAX_SENSOR_VALUE_STR ") cup: %d/127 (%d/" MAX_SENSOR_VALUE_STR ")\n",
        name.c_str(),
        hitIndex == 0 ? "Bow" : (hitIndex == 1 ? "Edge" : "Cup"),
        hitVelocities[MAIN_PIEZO_INDEX], maxZoneValues[MAIN_PIEZO_INDEX],
        (zoneCount >= 2 ? hitVelocities[1] : 0), (zoneCount >= 2 ? maxZoneValues[1] : 0),
        (zoneCount >= 3 ? hitVelocities[2] : 0), (zoneCount >= 3 ? maxZoneValues[2] : 0));
  }

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

static unsigned int findMaxValueIndex(sensor_value_t* values, unsigned int count) {
  sensor_value_t maxValue = values[0];
  unsigned int maxIndex = 0;
  for (unsigned int index = 1; index < count; ++index) {
    if (values[index] > maxValue) {
      maxValue = values[index];
      maxIndex = index;
    }
  }
  return maxIndex;
}

zone_size_t PiezoSwitchSensing::findZoneHitIndex() {
  const zone_size_t zoneCount = pad.getActiveZoneCount();
  if (zoneCount <= SWITCH_ZONE_OFFSET) {
    return MAIN_PIEZO_INDEX; // cymbal has no switches, must be main zone
  }
  zone_size_t switchCount = zoneCount - SWITCH_ZONE_OFFSET;

  sensor_value_t* switchPressCounters = &pad.maxZoneValues[SWITCH_ZONE_OFFSET];
  zone_size_t switchHitIndex = findMaxValueIndex(switchPressCounters, switchCount);
  return (switchPressCounters[switchHitIndex] > 0)
    ? switchHitIndex + SWITCH_ZONE_OFFSET
    : MAIN_PIEZO_INDEX;
}
