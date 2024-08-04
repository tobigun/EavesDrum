// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "drum_settings.h"

midi_velocity_t curve(sensor_value_t value, CurveType curveType);

midi_velocity_t scaleAndCurve(sensor_value_t value, sensor_value_t minValue, sensor_value_t maxValue, CurveType curveType);

sensor_value_t scale(sensor_value_t in_value, sensor_value_t in_min, sensor_value_t in_max) ;
