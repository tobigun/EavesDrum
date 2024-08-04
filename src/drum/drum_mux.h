// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>
#include "drum_io.h"

#define MAX_CHANNEL_COUNT 16 // max. 16 channels per mux

enum class MuxType {
  HC4051,
  HC4067,
  Unknown,
};

class DrumMux {
public:
  DrumMux() = default;

  // disable shallow copies
  DrumMux(const DrumMux&) = delete;
  DrumMux& operator=(const DrumMux&) = delete;

  // enable move semantic
  DrumMux(DrumMux&& other) = default;
  DrumMux& operator=(DrumMux&& other) = default;

public:
  void initHC4051(pin_size_t selectPin0, pin_size_t selectPin1, pin_size_t selectPin2, pin_size_t analogInPin, pin_size_t enablePin = PIN_UNUSED) {
    init(3, {selectPin0, selectPin1, selectPin2}, analogInPin, enablePin);
  }

  void initHC4067(pin_size_t selectPin0, pin_size_t selectPin1, pin_size_t selectPin2, pin_size_t selectPin3, pin_size_t analogInPin, pin_size_t enablePin = PIN_UNUSED) {
    init(4, {selectPin0, selectPin1, selectPin2, selectPin3}, analogInPin, enablePin);
  }

public:
  MuxType getMuxType() const {
    return selectPins[3] == PIN_UNUSED ? MuxType::HC4051 : MuxType::HC4067;
  }

  channel_size_t getChannelCount() const { return channelCount; }

  sensor_value_t readChannel(channel_size_t channelIndex) const { return channelBuffer[channelIndex]; }
  
  void scan();

  pin_size_t getAnalogInPin() const { return analogInPin; }
  pin_size_t getEnablePin() const { return enablePin; }

  pin_size_t getSelectPinsCount() const { return selectPinsCount; }
  pin_size_t getSelectPin(pin_size_t index) const { return selectPins[index]; }

  operator String() const;

private:
  void init(pin_size_t selectPinsCount, const pin_size_t (&selectPins)[4], pin_size_t analogInPin, pin_size_t enablePin);

  void setMuxEnabled(bool enable);

  void selectChannel(channel_size_t channel);
  
private:
  bool initialized = false;

  pin_size_t analogInPin;
  pin_size_t enablePin;
  pin_size_t channelCount;

  pin_size_t selectPinsCount;
  pin_size_t selectPins[4];

  sensor_value_t channelBuffer[MAX_CHANNEL_COUNT];
};
