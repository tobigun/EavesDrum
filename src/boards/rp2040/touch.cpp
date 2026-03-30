// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "touch.h"

#include "capsense.pio.h"
#include "log.h"

#include <Arduino.h>

#define CALIBRATION_DURATION_MS 2000
#define MIN_SWITCH_TIME_US 1000

TouchSensorManager touchSensorManager;

constexpr uint8_t oversamplingCount = 2;

void TouchSensorManager::addSensor(DrumPin& pin) { // TODO: handle multiple pins
  gpio_init(pin.index);
  gpio_pull_up(pin.index);

  programOffset = pio_add_program(pio, &capsense_program);

  pio_sm_config config = capsense_program_get_default_config(programOffset);

  // Map the state machine's OUT pin group to one pin, namely the `pin` parameter to this function.
  sm_config_set_set_pins(&config, pin.index, 1);
  sm_config_set_jmp_pin(&config, pin.index);
  sm_config_set_mov_status(&config, STATUS_RX_LESSTHAN, 1);
  sm_config_set_in_shift(&config, false, false, 32);

  // Set this pin's GPIO function (connect PIO to the pad)
  pio_gpio_init(pio, pin.index);

  // Load our configuration, and jump to the start of the program
  pio_sm_init(pio, stateMachineId, programOffset, &config);

  // Set the state machine running
  pio_sm_set_enabled(pio, stateMachineId, true);
  
  TouchSensorInfo& touchInfo = touchPins[&pin];
  touchInfo = TouchSensorInfo();
  startCalibration(touchInfo);

  // Start first run (and block afterwards)
  triggerNextSample();
}

void TouchSensorManager::removeSensor(DrumPin& pin) { // TODO: handle multiple pins
  bool found = touchPins.erase(&pin) > 0;
  if (!found) {
    logWarn("TouchSensorManager: Could not find pin %d to remove", pin.index);
    return;
  }

  pio_remove_program(pio, &capsense_program, programOffset);
}

// start sampling once and stop afterwards
void TouchSensorManager::triggerNextSample() {
  pio_sm_put(pio, stateMachineId, oversamplingCount);
}

uint32_t TouchSensorManager::tryReadSensor(DrumPin& pin) {
  if (pio_sm_is_rx_fifo_empty(pio, stateMachineId)) {
    return TOUCH_UNDEFINED;
  }

  // drain FIFO, keeping only the last value (usually there will only be one value)
  while (pio_sm_get_rx_fifo_level(pio, stateMachineId) > 1) {
    pio_sm_get(pio, stateMachineId);
  }

  return UINT32_MAX - pio_sm_get(pio, stateMachineId);
}

void TouchSensorManager::updateCalibration(TouchSensorInfo& touchInfo, uint32_t value) {
  TouchCalibrationInfo& calibration = touchInfo.calibration;

  if (value < calibration.minValue) {
    calibration.minValue = value;
  } else if (value > calibration.maxValue) {
    calibration.maxValue = value;
  }

  if (millis() - calibration.startTimeMs > CALIBRATION_DURATION_MS) {
    calibration.needsCalibration = false;
    uint32_t avg = (calibration.minValue + calibration.maxValue) / 2;
    uint32_t deviation = (calibration.maxValue - calibration.minValue) / 2;
    touchInfo.switchOnThreshold= avg + deviation * 1.5;
    touchInfo.switchOffThreshold= avg + deviation * 1.1;

    logInfo("Touch calibration: [%ld, %ld] %ld, %ld", calibration.minValue, calibration.maxValue,
      touchInfo.switchOnThreshold, touchInfo.switchOffThreshold);
  }
}

static inline void checkStateSwitched(TouchSensorInfo& touchInfo, bool newState, uint32_t value) {
  if (touchInfo.switchStartUs == 0) {
     // switch state transition detected, wait if new state is stable
    touchInfo.switchStartUs = micros();
    return;
  }

  // last time we already detected an on state. Make sure it is not just a fluctuation
  if (micros() - touchInfo.switchStartUs > MIN_SWITCH_TIME_US) {
    touchInfo.isOn = newState;
    touchInfo.switchStartUs = 0;
    //logInfo("Touch: %d (%d)\n", touchInfo.isOn, value);
  }
}

bool TouchSensorManager::readSensor(DrumPin& pin) { // TODO: handle multiple pins
  TouchSensorInfo& touchInfo = touchPins[&pin];

  // read next value
  uint32_t rawValue = tryReadSensor(pin);
  if (rawValue == TOUCH_UNDEFINED) {
    return touchInfo.isOn;
  }
  uint32_t value = rawValue / oversamplingCount;

  if (touchInfo.calibration.needsCalibration) {
    updateCalibration(touchInfo, value);
  }

  if (!touchInfo.isOn && (value > touchInfo.switchOnThreshold)) {
    checkStateSwitched(touchInfo, true, value);
  } else if (touchInfo.isOn && (value < touchInfo.switchOffThreshold)) {
    checkStateSwitched(touchInfo, false, value);
  } else {
    // reset switching transition time.
    // If we formerly detected the start of a state switch, it seems to just have been a fluctuation
    touchInfo.switchStartUs = 0;
  }

  triggerNextSample();

  return touchInfo.isOn;
}
