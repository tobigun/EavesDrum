// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"

#include "event_log.h"

const char* levelToString(Level level) {
  switch (level) {
    case Level::None: return "";
    case Level::Debug: return "DEBUG: ";
    case Level::Info: return "INFO: ";
    case Level::Error: return "ERROR: ";
    case Level::Warn: return "WARN: ";
    default: return "";
  }  
}

void logString(Level level, String message) {
  SerialDebug.printf("%s%s\n", levelToString(level), message.c_str());
}

static void logAtLevel(Level level, const char* format, va_list args) {
  char formatBuffer[128];
  vsnprintf(formatBuffer, sizeof(formatBuffer), format, args);
  SerialDebug.printf("%s%s\n", levelToString(level), formatBuffer);
}

void logPrintf(const char* format, ...) {
  va_list args;
  va_start(args, format);
  logAtLevel(Level::None, format, args);
  va_end(args);
}

void logDebug(const char* format, ...) {
  va_list args;
  va_start(args, format);
  logAtLevel(Level::Debug, format, args);
  va_end(args);
}

void logInfo(const char* format, ...) {
  va_list args;
  va_start(args, format);
  logAtLevel(Level::Info, format, args);
  va_end(args);
}

void logWarn(const char* format, ...) {
  va_list args;
  va_start(args, format);
  logAtLevel(Level::Warn, format, args);
  va_end(args);
}

void logError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  logAtLevel(Level::Error, format, args);
  va_end(args);
}
