// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef ESP32

#include <Arduino.h>
#include "drum_io.h"

void DrumIO::setup(bool usePwmPowerSupply) {
  analogReadResolution(12); // IMPORTANT: code for ADC2 has a bug that applies the mapping twice if res!=12 
  //analogSetAttenuation(ADC_0db);
}

bool DrumIO::initAnalogInPin(pin_size_t pin) {
  analogSetPinAttenuation(pin, ADC_11db);
  return true;
}

static void readAnalogInPinInternal(pin_size_t pin, uint16_t *captureBuffer, uint8_t numSamples) {
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

bool DrumIO::initDigitalOutPin(pin_size_t pin)
{
  if (pin >= __GPIOCNT) {
    return false;
  }
  pinMode(pin, OUTPUT);
  return true;
}

void DrumIO::writeDigitalOutPin(pin_size_t pinNumber, pin_state_t status) {
  digitalWrite(pinNumber, status);
}

#endif
