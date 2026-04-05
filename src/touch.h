
// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>
#include <map>
#include "drum_pin.h"

class TouchSensorManager {
public:
  void addSensor(DrumPin& pin);
  void removeSensor(DrumPin& pin);

  bool readSensor(DrumPin& pin);
};

extern TouchSensorManager touchSensorManager;
