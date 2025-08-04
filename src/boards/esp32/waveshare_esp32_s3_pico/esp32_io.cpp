// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_io.h"
#include "log.h"

#include <Arduino.h>

#define NUMPIXELS 1
#define BRIGHTNESS 12

static bool ledEnabled[] = {false, false, false};

static void ledUpdateColor();

void DrumIO::setup(bool usePwmPowerSupply) {
  ledUpdateColor();
  analogReadResolution(12); // IMPORTANT: code for ADC2 has a bug that applies the mapping twice if res!=12
  // analogSetAttenuation(ADC_0db);
}

bool DrumIO::initAnalogInPin(pin_size_t pin) {
  analogSetPinAttenuation(pin, ADC_11db);
  return true;
}

static void readAnalogInPinInternal(pin_size_t pin, uint16_t* captureBuffer, uint8_t numSamples) {
  for (byte i = 0; i < numSamples; ++i) {
    captureBuffer[i] = analogRead(pin);
  }
}

sensor_value_t DrumIO::readAnalogInPin(pin_size_t pin) {
  uint16_t samples[NUM_SAMPLES];
  readAnalogInPinInternal(pin, samples, NUM_SAMPLES);

  uint16_t sum = 0;
  for (byte i = 0; i < NUM_SAMPLES; ++i) {
    sum += samples[i];
  }

  return (sum / NUM_SAMPLES) >> 2; // 12 to 10 bit
}

bool DrumIO::initDigitalOutPin(pin_size_t pin) {
  if (pin >= GPIO_PIN_COUNT) { // GPIO_NUM_MAX
    return false;
  }
  pinMode(pin, OUTPUT);
  return true;
}

void DrumIO::writeDigitalOutPin(pin_size_t pinNumber, pin_status_t status) {
  digitalWrite(pinNumber, status);
}

static void ledUpdateColor() {
  // TODO: rgbLedWrite() takes quite long. Use normal LEDs instead
  rgbLedWrite(RGB_BUILTIN,
      ledEnabled[0] ? BRIGHTNESS : 0,
      ledEnabled[1] ? BRIGHTNESS : 0,
      ledEnabled[2] ? BRIGHTNESS : 0);
}

void DrumIO::led(uint8_t id, bool enable) {
  if (id >= 3) {
    return;
  }

  if (ledEnabled[id] != enable) {
    ledEnabled[id] = enable;
    ledUpdateColor();
  }
}

void DrumIO::reset() {
  SerialDebug.println("Reset");
  esp_restart();
}
