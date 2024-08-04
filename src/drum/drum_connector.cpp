// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_connector.h"

#include "event_log.h"

static void initSensor(const DrumPin& pin) {
  if (!pin.mux) {
    if (!DrumIO::initAnalogInPin(pin.index)) {
      eventLog.log(Level::ERROR, String("Mux: cannot initialize analog in pin: ") + pin);
    }
  }
}

void DrumConnector::setPins(const DrumPin* pins, pin_size_t pinCount) {
  for (pin_size_t i = 0; i < pinCount; ++i) {
    this->sensorPins[i] = pins[i];
    initSensor(pins[i]);
  }
  sensorPinsCount = pinCount;
}
