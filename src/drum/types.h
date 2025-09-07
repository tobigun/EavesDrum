// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>

#define MAX_SENSOR_VALUE ((sensor_value_t) 1023)
#define INVALID_SENSOR_VALUE UINT16_MAX

#define MAX_MIDI_NOTE 127
#define MIDI_NOTE_UNASSIGNED 255

#define BOOL_UNDEFINED -1
typedef int8_t bool_or_undefined;

typedef uint16_t sensor_value_t;
typedef uint8_t midi_velocity_t;
typedef uint8_t midi_note_t;
typedef uint8_t zone_size_t;
typedef uint8_t channel_size_t;
typedef uint8_t pad_size_t;
typedef uint8_t mappings_size_t;
typedef uint8_t mux_size_t;
typedef uint8_t connector_size_t;

#ifndef pin_size_t
typedef uint8_t pin_size_t;
typedef uint8_t pin_status_t;
#endif

#ifdef __x86_64
typedef uint64_t time_base_t;
#else
typedef uint32_t time_base_t;
#endif
typedef time_base_t time_ms_t;
typedef time_base_t time_us_t;
