// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scale.h"
#include <math.h>

/**
 * @param value input value (1 .. 1023)
 * @result value adjusted by curve function (1 .. 127)
 */
static inline midi_velocity_t curveFunc(sensor_value_t inputValue, float base) {
  const sensor_value_t MAX_INPUT_VALUE = MAX_SENSOR_VALUE;
  const midi_velocity_t MAX_OUTPUT_VALUE = 127;
  float curveValue = pow(base, inputValue - 1) - 1; // TODO: use table?
  float curveMax = pow(base, MAX_INPUT_VALUE - 1) - 1;
  float outputValue = curveValue / curveMax * (MAX_OUTPUT_VALUE - 1);
  return round(outputValue) + 1;
}

sensor_value_t scale(sensor_value_t in_value, sensor_value_t in_min, sensor_value_t in_max) {
  const sensor_value_t out_max = MAX_SENSOR_VALUE;
  if (in_value == 0 || in_value < in_min) {
    return 0;
  } else if (in_value >= in_max) {
    return out_max;
  }

  const sensor_value_t out_min = 1; // map value == minValue to 1 and not 0
  sensor_value_t scaledValue = (uint32_t)(in_value - in_min) * (out_max - out_min) / (in_max - in_min);
  return scaledValue + out_min;
}

midi_velocity_t curve(sensor_value_t value, CurveType curveType) {
  if (value == 0) {
    return 0;
  }

  switch (curveType) {
  case CurveType::Linear: {
    sensor_value_t result = ((value - 1) >> 3) + 1;  // 10 to 7 bit
    return result == 128 ? 127 : result;
  }
  case CurveType::Exponential1:
    return curveFunc(value, 1.002);
  case CurveType::Exponential2:
    return curveFunc(value, 1.004);
  case CurveType::Logarithmic1:
    return curveFunc(value, 0.998);
  case CurveType::Logarithmic2:
    return curveFunc(value, 0.996);
  default:
    return 0;
  };
}

midi_velocity_t scaleAndCurve(sensor_value_t value, sensor_value_t minValue, sensor_value_t maxValue, CurveType curveType) {
  sensor_value_t scaledValue = scale(value, minValue, maxValue);
  if (scaledValue == 0) {
    return scaledValue;
  }

  switch (curveType) {
  case CurveType::Linear: {
    sensor_value_t result = ((scaledValue - 1) >> 3) + 1;  // 10 to 7 bit
    return result == 128 ? 127 : result;
  }
  case CurveType::Exponential1:
    return curveFunc(scaledValue, 1.002);
  case CurveType::Exponential2:
    return curveFunc(scaledValue, 1.004);
  case CurveType::Logarithmic1:
    return curveFunc(scaledValue, 0.998);
  case CurveType::Logarithmic2:
    return curveFunc(scaledValue, 0.996);
  default:
    return 0;
  };
}
