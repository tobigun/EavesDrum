// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>

#ifdef ARDUINO_ARCH_RP2040
#define SerialDebug Serial1 // Serial
#else
#define SerialDebug Serial
#endif
#ifdef EDRUM_DEBUG_ENABLED
#define EDRUM_DEBUG SerialDebug.printf
#define EDRUM_DEBUGLN SerialDebug.printf
#else
#define EDRUM_DEBUG(...)
#define EDRUM_DEBUGLN(...)
#endif

void logError(String error);
