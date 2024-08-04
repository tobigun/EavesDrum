// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "touch.h"

#include "capsense.pio.h"
#include "log.h"

#include <Arduino.h>

//#define PROFILE_CAPSENSE

TouchSensor::TouchSensor(pin_size_t pin) : pin(pin) {
  gpio_init(pin);
  gpio_pull_up(pin);

  programOffset = pio_add_program(pio, &capsense_program);
  init();
}

TouchSensor::~TouchSensor() {
  pio_remove_program(pio, &capsense_program, programOffset);
}

void TouchSensor::init() {
  pio_sm_config c = capsense_program_get_default_config(programOffset);

  // Map the state machine's OUT pin group to one pin, namely the `pin` parameter to this function.
  sm_config_set_set_pins(&c, pin, 1);
  sm_config_set_jmp_pin(&c, pin);
  sm_config_set_mov_status(&c, STATUS_RX_LESSTHAN, 1);
  sm_config_set_in_shift(&c, false, false, 32);

  // Set this pin's GPIO function (connect PIO to the pad)
  pio_gpio_init(pio, pin);

  // Load our configuration, and jump to the start of the program
  pio_sm_init(pio, stateMachineId, programOffset, &c);

  // Set the state machine running
  pio_sm_set_enabled(pio, stateMachineId, true);
}

uint32_t TouchSensor::readTouch() {
  //pio_sm_set_enabled(pio, stateMachineId, true);

  pio_sm_put(pio, stateMachineId, oversamplingCount);
  uint32_t level = UINT32_MAX - pio_sm_get_blocking(pio, stateMachineId);

  // Disable the state machines to reduce noise and interference induced by the PIO pulling the GPIO pins.
  // This can be particularly important for complying for EMC standards.
  //pio_sm_set_enabled(pio, stateMachineId, false);

  return level / oversamplingCount;
}

uint32_t TouchSensor::sense() {
#ifdef PROFILE_CAPSENSE
  uint32_t startTimeUs = micros();
#endif
  uint32_t level = readTouch();
#ifdef PROFILE_CAPSENSE
  uint32_t endTimeUs = micros();
  SerialDebug.printf("Touch: %d, %d us\n", level, endTimeUs - startTimeUs);
#endif
  return level;
}
