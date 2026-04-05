// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_connector.h"

#include "event_log.h"

void DrumConnector::start() {
  for (pin_size_t i = 0; i < sensorPinsCount; ++i) {
    DrumPin& sensorPin = sensorPins[i];
    if (!sensorPin.mux) {
      if (!DrumIO::initAnalogInPin(sensorPin.index)) {
        eventLog.log(Level::Error, String("Input: cannot initialize analog in pin: ") + sensorPin.index);
      }
    }
  }
}

void DrumConnector::setPins(const DrumPin* pins, pin_size_t pinCount) {
  for (pin_size_t i = 0; i < pinCount; ++i) {
    sensorPins[i] = pins[i];
  }
  sensorPinsCount = pinCount;
}
