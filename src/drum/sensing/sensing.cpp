// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_pad.h"
#include "log.h"

#include "sensing/sensing.h"
#include "sensing/scale.h"

const uint32_t MAX_PREFERENCE_MULT = 5;


void PiezoSensing::sense(time_us_t senseTimeUs) {
  if (!pad.isConnectorActive()) {
    return;
  }

  resetHitInfo();
  readInputValues();

  if (pad.sensingState == SensingState::PeakDetect) { // check for first peak
    pad.sensingState = detectPeak(senseTimeUs, pad.settings.zoneThresholdsMin)
      ? SensingState::Scan : SensingState::PeakDetect;
  } else if (pad.sensingState == SensingState::Scan) { // search highest peak
    pad.sensingState = scan(senseTimeUs);
  } else if (pad.sensingState == SensingState::Mask) {
    time_us_t timeSinceMaskBeginUs = senseTimeUs - pad.scanTimeEndUs;
    bool maskActive = timeSinceMaskBeginUs < pad.settings.maskTimeMs * 1000;
    if (!maskActive) {
      pad.sensingState = (pad.settings.decayTimeMs > 0) ? SensingState::Decay : SensingState::PeakDetect;
    }
  } else if (pad.sensingState == SensingState::Decay) {
    time_us_t maskEndTimeUs = pad.scanTimeEndUs + pad.settings.maskTimeMs * 1000;
    time_us_t timeSinceDecayBeginUs = senseTimeUs - maskEndTimeUs;
    bool decayActive = timeSinceDecayBeginUs < pad.settings.decayTimeMs * 1000;
    if (!decayActive) {
      pad.sensingState = SensingState::PeakDetect;
    } else {
      float decayProgress = (float) timeSinceDecayBeginUs / (pad.settings.decayTimeMs * 1000);
      pad.sensingState = detectPeakWithDecay(senseTimeUs, decayProgress)
      ? SensingState::Scan : SensingState::Decay;
    }
  }
}

void Sensing::resetHitInfo() {
  const zone_size_t zoneCount = pad.getActiveZoneCount();
  for (zone_size_t zone = 0; zone < zoneCount; ++zone) {
    pad.hits[zone] = false;
    pad.hitVelocities[zone] = 0;
  }
  pad.cymbal.isChoked = false;
}

void Sensing::readInputValues() {
  const pin_size_t pinCount = pad.getActivePinCount();
  for (pin_size_t pinIndex = 0; pinIndex < pinCount; ++pinIndex) {
    bool autoCalibrate = (pad.getPadType() == PadType::Drum) ? pad.autoCalibrate : false;
    sensor_value_t inputValue = pad.readInput(pad.connector->getPin(pinIndex),
       autoCalibrate ? InputFlags::AUTO_CALIBRATE : InputFlags::NONE);

    if (pad.settings.zonesType == ZonesType::Zones3_PiezoAndSwitches_1TRS && pinIndex == 1) {
      uint8_t sensorIndex = (inputValue >= pad.settings.zoneThresholdsMin[2]) ? 2 : 1;
      pad.sensorValues[sensorIndex] = inputValue;
      pad.sensorValues[3 - sensorIndex] = 0; // set the other zone to 0
    } else {
      pad.sensorValues[pinIndex] = inputValue;
    }
  }
}

bool Sensing::isChoked(zone_size_t activeZoneCount) {
  if (pad.getPadType() != PadType::Cymbal || pad.settings.chokeType == ChokeType::None) {
    return false;
  }

  if (pad.settings.chokeType == ChokeType::TouchSensor) {
    DrumConnector* touchSensor = pad.getTouchSensor();
    if (touchSensor) {
      return touchSensorManager.readSensor(touchSensor->getPin(0));
    }
    return false;
  }

  zone_size_t chokeSwitchZoneIndex = (pad.settings.chokeType == ChokeType::Switch_Edge) ? 1 : 2;
  if (activeZoneCount > chokeSwitchZoneIndex) {
    return isZoneSwitchPressed(chokeSwitchZoneIndex, activeZoneCount);
  }

  return false;
}

void Sensing::logHit(zone_size_t hitIndex, zone_size_t zoneCount) {
#ifdef EDRUM_DEBUG_ENABLED
  if (pad.getPadType() == PadType::Drum) {
    EDRUM_DEBUG("[Hit '%s' %s] head: %d/127 (%d/" MAX_SENSOR_VALUE_STR ") rim: %d/127 (%d/" MAX_SENSOR_VALUE_STR ")\n",
      pad.name.c_str(),
      hitIndex == 0 ? "Head" : (hitIndex == 1 ? "Rim" : "Side-Rim"),
      pad.hitVelocities[0], pad.maxZoneValues[0],
      (zoneCount >= 2 ? hitVelocities[1] : 0), (zoneCount >= 2 ? maxZoneValues[1] : 0));
  } else {
    EDRUM_DEBUG("[Hit '%s' %s] bow: %d/127 (%d/" MAX_SENSOR_VALUE_STR ") edge: %d/127 (%d/" MAX_SENSOR_VALUE_STR ") cup: %d/127 (%d/" MAX_SENSOR_VALUE_STR ")\n",
        pad.name.c_str(),
        hitIndex == 0 ? "Bow" : (hitIndex == 1 ? "Edge" : "Cup"),
        pad.hitVelocities[MAIN_PIEZO_INDEX], pad.maxZoneValues[MAIN_PIEZO_INDEX],
        (zoneCount >= 2 ? pad.hitVelocities[1] : 0), (zoneCount >= 2 ? pad.maxZoneValues[1] : 0),
        (zoneCount >= 3 ? pad.hitVelocities[2] : 0), (zoneCount >= 3 ? pad.maxZoneValues[2] : 0));
  }
#endif
}

SensingState Sensing::handleChoked() {
  if (pad.cymbal.lastEventType == LastCymbalEventType::Choked) {
    // do not choke again if already choked.
    // This will enable us to detect choked hits that would otherwise be masked by the choke events
    return SensingState::PeakDetect;
  }

  pad.cymbal.isChoked = true;
  pad.cymbal.lastEventType = LastCymbalEventType::Choked;
  EDRUM_DEBUG("[Choked '%s'] \n", name.c_str());
  return SensingState::Mask;
}

bool Sensing::isZoneSwitchPressed(zone_size_t zone, zone_size_t activeZoneCount) {
  return pad.sensorValues[zone] >= pad.settings.zoneThresholdsMin[zone];
}

zone_size_t Sensing::findMaxValueIndex(uint16_t* values, zone_size_t zoneCount, zone_size_t preferedIndex) {
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

// returns true if peak was detected
bool PiezoSensing::detectPeak(time_us_t senseTimeUs, sensor_value_t* zoneThresholdsMin) {
  const zone_size_t activeZoneCount = pad.getActiveZoneCount();

  bool isPeakDetected = false;
  for (zone_size_t zone = 0; zone < activeZoneCount; ++zone) {
    bool isPeak = pad.sensorValues[zone] >= zoneThresholdsMin[zone];
    isPeakDetected |= isPeak;
  }

  bool isMaybeChoked = isChoked(activeZoneCount); // we cannot be sure the cymbal is choked as a hit might still be detected

  if (!isPeakDetected && !isMaybeChoked) {
    return false;
  }

  // peak detected or cymbal might be choked -> start scan time
  pad.hitTimeUs = senseTimeUs;

  for (zone_size_t zone = 0; zone < activeZoneCount; ++zone) {
    pad.maxZoneValues[zone] = 0;
  }
  updateMaxSensorValue(activeZoneCount);

  return true;
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
  updateMaxSensorValue(zoneCount);

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
  if (pad.maxZoneValues[hitIndex] == 0) { // choked
    return handleChoked();
  }

  pad.hits[hitIndex] = true;
  pad.cymbal.lastEventType = LastCymbalEventType::Hit;
  logHit(hitIndex, zoneCount);

  return SensingState::Mask;
}

// decayProgress: progress of decay in p.u. (0.0 .. 1.0). 0: start of decay, 1: end of decay 
// returns true if peak detected
bool PiezoSensing::detectPeakWithDecay(time_us_t senseTimeUs, float decayProgress) {
  const zone_size_t activeZoneCount = pad.getActiveZoneCount();
  sensor_value_t zoneThresholdsMin[3];
  for (zone_size_t zone = 0; zone < activeZoneCount; ++zone) {
    sensor_value_t maxValue = pad.maxZoneValues[zone];
    sensor_value_t defaultThresholdMin = pad.settings.zoneThresholdsMin[zone];

    zoneThresholdsMin[zone] = (maxValue > defaultThresholdMin)
      ? maxValue - (maxValue - defaultThresholdMin) * decayProgress
      : defaultThresholdMin;
  }

  return detectPeak(senseTimeUs, zoneThresholdsMin);
}

inline void PiezoSensing::updateMaxSensorValue(zone_size_t activeZoneCount) {
  for (zone_size_t zone = 0; zone < activeZoneCount; ++zone) {
    const sensor_value_t piezoValue = pad.sensorValues[zone];
    if (piezoValue > pad.maxZoneValues[zone]
        && piezoValue >= pad.settings.zoneThresholdsMin[zone]) { // Note: threshold must be checked as pad could be choked, i.e. old value could be 0
      pad.maxZoneValues[zone] = piezoValue;
    }
  }
}
