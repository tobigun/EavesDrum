// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>

#define LOG_BAUD 115200

#ifdef ARDUINO_ARCH_RP2040
#define SerialDebug Serial1 // Serial
#else
#define SerialDebug Serial
#endif
#ifdef EDRUM_DEBUG_ENABLED
#define EDRUM_DEBUG logInfo
#define EDRUM_DEBUGLN logInfo
#else
#define EDRUM_DEBUG(...)
#define EDRUM_DEBUGLN(...)
#endif

enum class Level {
  Debug = 0,
  Info,
  Warn,
  Error,
  None,
};

enum class LogMode {
  Default,
  NoPrefixOrNewline,
  NoNewline
};

#ifdef ENABLE_SERIAL_DEBUG
#define DEFAULT_LOG_LEVEL Level::Info
#else
#define DEFAULT_LOG_LEVEL Level::None
#endif

#ifdef __cplusplus
extern "C" {
#endif

void logDebug(const char* format, ...);
void logInfo(const char* format, ...);
void logWarn(const char* format, ...);
void logError(const char* format, ...);

#ifdef __cplusplus
}
#endif

void logString(Level level, String message, LogMode mode = LogMode::Default);

const char* levelToString(Level level);

void setLogLevel(Level level);
