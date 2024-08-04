// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_io.h"
#include "drum_mux.h"

#include "log.h"

void DrumMux::init(pin_size_t selectPinsCount, const pin_size_t (&selectPins)[4], pin_size_t analogInPin, pin_size_t enablePin) {
  this->analogInPin = analogInPin;
  this->enablePin = enablePin;
  this->selectPinsCount = selectPinsCount;

  bool failed = false;

  channelCount = (1 << selectPinsCount);
  if (channelCount > MAX_CHANNEL_COUNT) {
    logError(String("Mux: #channels > ") + MAX_CHANNEL_COUNT);
    failed = true;
  }

  if (!DrumIO::initAnalogInPin(analogInPin)) {
    logError(String("Mux: cannot initialize analog in pin: ") + analogInPin);
    failed = true;
  }

  if (enablePin != PIN_UNUSED) {
    if (DrumIO::initDigitalOutPin(enablePin)) {
      DrumIO::writeDigitalOutPin(enablePin, HIGH);
    } else {
      logError(String("Mux: cannot initialize enable pin: ") + enablePin);
      failed = true;
    }
  }

  for (pin_size_t i = 0; i < selectPinsCount; i++) {
    this->selectPins[i] = selectPins[i];
    if (DrumIO::initDigitalOutPin(selectPins[i])) {
      DrumIO::writeDigitalOutPin(selectPins[i], HIGH);
    } else {
      logError(String("Mux: cannot initialize select pin: ") + selectPins[i]);
      failed = true;
    }
  }

  initialized = !failed;
}

#ifdef ARDUINO_ARCH_RP2040
// switch-on-time for 3.3V between 45-225ns -> ~250ns. Delay might not be necessary
#define MUX_SWITCH_ON_DELAY_CPU_CYCLES (250 * (F_CPU / 1000000L) / 1000)
#define WAIT_UNTIL_MUX_STABLE() busy_wait_at_least_cycles(MUX_SWITCH_ON_DELAY_CPU_CYCLES)
#else
#define WAIT_UNTIL_MUX_STABLE() {}
#endif

inline void DrumMux::setMuxEnabled(bool enable) {
  if (enablePin != PIN_UNUSED) {
    DrumIO::writeDigitalOutPin(enablePin, enable ? LOW : HIGH);
  }
}

inline void DrumMux::selectChannel(channel_size_t channel) {
  for (pin_size_t selectPinIndex = 0; selectPinIndex < selectPinsCount; selectPinIndex++) {
    DrumIO::writeDigitalOutPin(selectPins[selectPinIndex], ((channel & (1 << selectPinIndex)) ? HIGH : LOW));
  }
}

// overwritten by simulation
sensor_value_t __attribute__((weak)) readMuxChannel(channel_size_t channel, pin_size_t analogInPin) {
  WAIT_UNTIL_MUX_STABLE();
  return DrumIO::readAnalogInPin(analogInPin);
}

void DrumMux::scan() {
  if (!initialized) {
    return;
  }

  for (channel_size_t channel = 0; channel < channelCount; channel++) {
    // switch channel when mux is disabled. Otherwise other channels may be read during switching
    selectChannel(channel);
    setMuxEnabled(true);
    channelBuffer[channel] = readMuxChannel(channel, analogInPin);
    setMuxEnabled(false);
  }
}

DrumMux::operator String() const {
  return String("Mux: ")
      + (getMuxType() == MuxType::HC4051 ? "HC4051" : "HC4067")
      + " Select-Pins: ["
      + selectPins[0] + " "
      + selectPins[1] + " "
      + selectPins[2]
      + (selectPins[3] != PIN_UNUSED ? String(" ") + selectPins[3] : String(" "))
      + "] AnalogIn-Pin: " + analogInPin
      + " Enable-Pin: " + (enablePin != PIN_UNUSED ? String(enablePin) : String("-"));
}
