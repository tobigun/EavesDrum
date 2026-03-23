// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>
#include "types.h"

#define PIN_UNUSED 0xFF
#define MUX_UNUSED 0xFF

#define NUM_SAMPLES 2

enum class LedId {
  HitIndicator,
  Network,
  MidiConnected,
  WatchDog,
};

enum class ButtonId {
  Wifi = 0
};

class DrumIO {
public:
  DrumIO() = delete;

public:
  static void update();

  static void setup(bool usePwmPowerSupply);

  static bool initAnalogInPin(pin_size_t pin);

  /** Reads a 10 bit ADC value from the given pin */
  static sensor_value_t readAnalogInPin(pin_size_t pin);

  static bool initDigitalOutPin(pin_size_t pin);

  static void writeDigitalOutPin(pin_size_t pin, pin_status_t status);

  static void led(LedId id, bool enable);

  static bool isButtonPressed(ButtonId id);

  /**
   * Request a hardware reset after the given delay.
   * This only returns if delayMs > 0 or if only a soft reset was performed instead (e.g. on PC).
   */
  static bool requestReset(uint32_t delayMs = 0);
};
