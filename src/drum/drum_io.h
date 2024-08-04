// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"

#define PIN_UNUSED 0xFF
#define MUX_UNUSED 0xFF

#define NUM_SAMPLES 2

#define LED_WATCHDOG 0
#define LED_HIT_INDICATOR 1
#define LED_NETWORK 2

class DrumIO {
public:
  DrumIO() = delete;

public:
  static void setup(bool usePwmPowerSupply);

  static bool initAnalogInPin(pin_size_t pin);

  /** Reads a 10 bit ADC value from the given pin */
  static sensor_value_t readAnalogInPin(pin_size_t pin);

  static bool initDigitalOutPin(pin_size_t pin);

  static void writeDigitalOutPin(pin_size_t pin, pin_status_t status);

  static void led(uint8_t id, bool enable);

  static void reset();
};
