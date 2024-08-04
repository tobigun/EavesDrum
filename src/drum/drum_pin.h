// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "drum_mux.h"

#define BALANCE_THRESHOLD 512

enum class SignalType {
  /**
   * Use the plain input without post processing.
   */
  Normal,

  /**
   * An ADC can only sample positive voltages if no external negative voltage reference is connected.
   * Hence, a voltage offset is added to the muxer output to shift the negative half-waves of the piezos into the positive
   * voltage range so that it can be measured.
   * The voltage offset must be calibrated by a poti so that 0V results in a value the middle of the value range of the ADC.
   */
  VoltageOffset,
};

struct DrumPin {
  const DrumMux* mux;
  mux_size_t muxIndex;
  uint8_t index;
  sensor_value_t offset = MAX_SENSOR_VALUE / 2;
  int16_t offsetBalance = 0;

  DrumPin()
    : mux(nullptr), muxIndex(MUX_UNUSED), index(PIN_UNUSED) {}

  DrumPin(pin_size_t pinIndex)
    : mux(nullptr), muxIndex(MUX_UNUSED), index(pinIndex) {}

  DrumPin(DrumMux* mux, mux_size_t muxIndex, channel_size_t channelIndex)
    : mux(mux), muxIndex(muxIndex), index(channelIndex) {}

  bool isValid() const {
    return index != PIN_UNUSED;
  }

  // TODO: required if hihat controller is directly connected to ADC.
  // No use case at the moment, as hi-hat controllers are now also connected to a muxer by default
  bool isInverted() const { return false; }

  /**
   * Returns the signal type.
   * @return either Normal or VoltageOffset.
   */
  SignalType getSignalType() const {
    // make the simple assumption that all muxers have a voltage offset and pins directly connected to an ADC not
    return mux != nullptr ? SignalType::VoltageOffset : SignalType::Normal;
  }

  sensor_value_t getOffset() const {
    return offset;
  }

  sensor_value_t updateOffset(sensor_value_t value) {
    if (value > offset) {
      ++offsetBalance;
    } else if (value < offset) {
      --offsetBalance;
    }

    if (offsetBalance >= BALANCE_THRESHOLD) {
      ++offset;
      offsetBalance = 0;
    } else if (offsetBalance <= -BALANCE_THRESHOLD) {
      --offset;
      offsetBalance = 0;
    }

    return offset;
  }

  operator String() const {
    return String(index) + (mux ? String("@") + muxIndex : "");
  }
};
