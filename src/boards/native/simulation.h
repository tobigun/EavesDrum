// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config/config_mapper.h"
#include "drum_kit.h"
#include "midi_device.h"
#include "monitor.h"
#include "network.h"
#include "webui.h"
#include "simulation.h"

#define ZERO_OFFSET (MAX_SENSOR_VALUE / 2)

extern DrumKit drumKit;

void setPadPinValue(const DrumPad& pad, zone_size_t zone, sensor_value_t value);
