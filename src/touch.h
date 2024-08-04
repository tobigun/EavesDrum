// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>

class TouchSensor {
public:
  TouchSensor(pin_size_t pin);
  ~TouchSensor();

  uint32_t sense();

private:
  void init();
  uint32_t readTouch();

private:
  const pin_size_t pin;
  const PIO pio = pio0;

  uint stateMachineId = 0;
  uint programOffset;

  uint8_t oversamplingCount = 2;
};
