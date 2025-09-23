// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"
#include "simulation.h"

#define MAX_PINS 50
sensor_value_t analogInValues[MAX_PINS];
sensor_value_t muxInValues[MAX_MUX_COUNT][MAX_CHANNEL_COUNT];

extern void handleReset();

void DrumIO::setup(bool usePwmPowerSupply) {
  for (int i = 0; i < MAX_PINS; ++i) {
    analogInValues[i] = 0; // assume analog in is just used for unsigned values
  }

  for (int mux = 0; mux < MAX_MUX_COUNT; ++mux) {
    for (int channel = 0; channel < MAX_CHANNEL_COUNT; ++channel) {
      muxInValues[mux][channel] = ZERO_OFFSET;
    }
  }
}

bool DrumIO::initAnalogInPin(pin_size_t analogInPin) {
  return true;
}

sensor_value_t DrumIO::readAnalogInPin(pin_size_t pin) {
  return analogInValues[pin];
}

// overwrites weak function in drum_mux.cpp
sensor_value_t readMuxChannel(channel_size_t channel, pin_size_t analogInPin) {
  for (int muxIndex = 0; muxIndex < drumKit.getMuxCount(); ++muxIndex) {
    const DrumMux& mux = *drumKit.getMux(muxIndex);
    if (mux.getAnalogInPin() == analogInPin) {
      return muxInValues[muxIndex][channel];
    }
  }
    
  return ZERO_OFFSET;
}

void setPadPinValue(const DrumPad& pad, zone_size_t zone, sensor_value_t value) {
  DrumConnector* connector = pad.getConnector();
  if (connector && zone < connector->getPinCount()) {
    const DrumPin& pin = connector->getPin(zone);
    if (pin.getSignalType() == SignalType::VoltageOffset) {
      muxInValues[pin.muxIndex][pin.index] = ZERO_OFFSET + value / 2;
    } else {
      analogInValues[pin.index] = value;
    }  
  }
}

bool DrumIO::initDigitalOutPin(pin_size_t pin) {
  return true;
}

void DrumIO::writeDigitalOutPin(pin_size_t pinNumber, pin_status_t status) {
}

void DrumIO::led(uint8_t id, bool enable) {
}

void DrumIO::reset() {
  SerialDebug.println("Reset");
  drumKit = DrumKit();
  DrumConfigMapper::loadAndApplyDrumKitConfig(drumKit);
}
