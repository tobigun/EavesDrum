
// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>
#include <map>
#include "drum_pin.h"

#define TOUCH_UNDEFINED UINT32_MAX

struct TouchCalibrationInfo {
  bool needsCalibration = true;
  uint32_t startTimeMs = 0;

  uint32_t minValue = TOUCH_UNDEFINED;
  uint32_t maxValue = 0;
};

struct TouchSensorInfo {
  TouchCalibrationInfo calibration;

  uint32_t switchOnThreshold;
  uint32_t switchOffThreshold;

  bool isOn = false;
  uint32_t switchStartUs = 0;
};

class TouchSensorManager {
public:
  void addSensor(DrumPin& pin);
  bool readSensor(DrumPin& pin);

  void removeSensor(DrumPin& pin);

private:
  void triggerNextSample();
  uint32_t tryReadSensor(DrumPin& pin);

  void startCalibration(TouchSensorInfo& touchInfo) {
    touchInfo.calibration = TouchCalibrationInfo();
    touchInfo.calibration.startTimeMs = millis();
    touchInfo.switchOnThreshold = TOUCH_UNDEFINED;
    touchInfo.switchOffThreshold = 0;
  }

  void updateCalibration(TouchSensorInfo& touchInfo, uint32_t value);

private:
  const PIO pio = pio0;

  uint stateMachineId = 0;
  uint programOffset;

  std::map<DrumPin*, TouchSensorInfo> touchPins;
};

extern TouchSensorManager touchSensorManager;
