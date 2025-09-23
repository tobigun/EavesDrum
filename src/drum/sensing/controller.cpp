// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_pad.h"
#include "drum_log.h"

#include "sensing/controller.h"
#include "sensing/scale.h"

void ControllerSensing::sense(time_us_t senseTimeUs) {
  if (!pad.isConnectorActive()) {
    return;
  }

  sensor_value_t rawInputValue = pad.readInput(pad.connector->getPin(0), InputFlags::INVERT);
  sensor_value_t oldDenoisedInputValue = pad.sensorValues[0];
  sensor_value_t denoisedInputValue = reduceNoise(rawInputValue, oldDenoisedInputValue, pad.settings.moveDetectTolerance);
  pad.sensorValues[0] = denoisedInputValue;
  pad.sensorValues[1] = rawInputValue;

  hiHatPedalPositionAndChickSensing(senseTimeUs, denoisedInputValue);
  hiHatPedalCCSensing(denoisedInputValue);
}

void ControllerSensing::hiHatPedalPositionAndChickSensing(time_us_t senseTimeUs, sensor_value_t pedalValue) {
  time_ms_t senseTimeMs = senseTimeUs / 1000;

  pad.hits[0] = false;
  pad.hitVelocities[0] = 0;

  const DrumSettings& settings = pad.settings;
  DrumPad::HiHatData& hihat = pad.hihat;

  const sensor_value_t range = settings.zoneThresholdsMax[0] - settings.zoneThresholdsMin[0];
  float pedalValuePercent = ((int16_t)pedalValue - settings.zoneThresholdsMin[0]) * 100.f / range;

  // closedThreshold > almostClosedThreshold > [open] >= 0
  bool isClosed = pedalValuePercent >= settings.closedThreshold;
  bool isAlmostClosed = !isClosed && (pedalValuePercent >= settings.almostClosedThreshold);
  bool isOpen = !isClosed && !isAlmostClosed;

  if (!isOpen && hihat.state == HiHatState::Open) { // closing: open -> almost closed
    hihat.almostClosedTimeMs = senseTimeMs;
    hihat.state = HiHatState::AlmostClosed;
  }

  if (isClosed && hihat.state == HiHatState::AlmostClosed) { // closing: almost closed -> completely closed
    time_ms_t closingTimeMs = senseTimeMs - hihat.almostClosedTimeMs;
    time_ms_t maxHitClosingTimeMs = settings.chickDetectTimeoutMs;

    // map closingTimeMs == 0 -> 127, closingTimeMs == maxHitClosingTimeMs -> 1, closingTimeMs > maxHitClosingTimeMs -> [0 .. -inf]
    int32_t velocity = map(closingTimeMs, maxHitClosingTimeMs, 0, 1, 127);
    pad.hitVelocities[0] = (velocity < 0) ? 0 : velocity;
    DEBUG_PRINTF("[Close] sensorValue: %d (%d%%) chick: %d/127 (%lu ms)\n", pedalValue, (int16_t)pedalValuePercent, hitVelocities[0], closingTimeMs);

    pad.hits[0] = pad.hitVelocities[0] > 0;
    hihat.state = HiHatState::Closed;
  }

  if ((!isClosed && hihat.state == HiHatState::Closed)
      || (isOpen && hihat.state == HiHatState::AlmostClosed)) { // opening: (almost) closed -> open
    DEBUG_PRINTF("[Open] sensorValue: %d (%d%%)\n", pedalValue, (int16_t)pedalValuePercent);
    hihat.state = HiHatState::Open;
  }
}

void ControllerSensing::hiHatPedalCCSensing(sensor_value_t pedalValue) {
  const DrumSettings& settings = pad.settings;
  DrumPad::HiHatData& hihat = pad.hihat;

  midi_velocity_t newPedalCC = scaleAndCurve(pedalValue,
    settings.zoneThresholdsMin[0],
    settings.zoneThresholdsMax[0],
    settings.curveType);
  if (newPedalCC != hihat.pedalCC) {
    DEBUG_PRINTF("[Move] pedalCC: %d/127 -> %d/127 (%d/" MAX_SENSOR_VALUE_STR "), hiHatClosed: %d\n", hihat.pedalCC, newPedalCC, pedalValue, hihat.state == HiHatState::Closed);

    hihat.pedalCC = newPedalCC;
    hihat.isMoving = true;
  } else {
    hihat.isMoving = false;
  }
}

sensor_value_t ControllerSensing::reduceNoise(sensor_value_t newValue, sensor_value_t oldValue, uint8_t moveDetectTolerance) {
  int signalDiff = abs(((int)oldValue) - newValue);
  if (newValue != oldValue && (signalDiff >= moveDetectTolerance || newValue == 0 || newValue >= MAX_SENSOR_VALUE)) {
    return min(newValue, MAX_SENSOR_VALUE);
  }
  return oldValue;
}
