// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "drum_pin.h"

#define MAX_SENSOR_PINS 3

typedef String ConnectorId;

class DrumConnector {
public:
  DrumConnector() = default;
  
  // disable shallow copies
  DrumConnector(const DrumConnector&) = delete;
  DrumConnector& operator=(const DrumConnector&) = delete;

  // enable move semantic
  DrumConnector(DrumConnector&& other) = default;
  DrumConnector& operator=(DrumConnector&& other) = default;

public:
  const String& getId() const { return id; }
  void setId(String id) { this->id = id; }

  void setPins(const DrumPin* pins, pin_size_t pinCount);

  pin_size_t getPinCount() const { return sensorPinsCount; }
  DrumPin& getPin(pin_size_t index) { return sensorPins[index]; }
  const DrumPin& getPin(pin_size_t index) const { return sensorPins[index]; }

private:
  ConnectorId id;

  pin_size_t sensorPinsCount = 0;
  DrumPin sensorPins[MAX_SENSOR_PINS];
};
