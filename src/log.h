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
#define EDRUM_DEBUG logDebug
#define EDRUM_DEBUGLN logDebug
#else
#define EDRUM_DEBUG(...)
#define EDRUM_DEBUGLN(...)
#endif

enum class Level {
  None,
  Debug,
  Info,
  Warn,
  Error
};

#ifdef __cplusplus
extern "C" {
#endif

void logPrintf(const char* format, ...);
void logDebug(const char* format, ...);
void logInfo(const char* format, ...);
void logWarn(const char* format, ...);
void logError(const char* format, ...);

#ifdef __cplusplus
}
#endif

void logString(Level level, String message);

const char* levelToString(Level level);
