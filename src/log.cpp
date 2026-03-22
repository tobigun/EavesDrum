// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"

#include "event_log.h"

static Level minLogLevel = DEFAULT_LOG_LEVEL;

const char* levelToString(Level level) {
  switch (level) {
  case Level::Debug:
    return "DEBUG: ";
  case Level::Info:
    return "INFO: ";
  case Level::Error:
    return "ERROR: ";
  case Level::Warn:
    return "WARN: ";
  default:
    return "";
  }
}

void setLogLevel(Level level) {
  minLogLevel = level;
}

static void logCharBuffer(Level level, const char* message, LogMode mode) {
  SerialDebug.printf("%s%s%s",
    mode == LogMode::NoPrefixOrNewline ? "" : levelToString(level),
    message,
    mode == LogMode::Default ? "\n" : "");
}

void logString(Level level, String message, LogMode mode) {
  if (minLogLevel != Level::None && level >= minLogLevel) {
    logCharBuffer(level, message.c_str(), mode);
  }
}

#define LOG_PRINTF(level) \
  if (level < minLogLevel) return; \
  va_list args; \
  va_start(args, format); \
  char formatBuffer[256]; \
  vsnprintf(formatBuffer, sizeof(formatBuffer), format, args); \
  logCharBuffer(level, formatBuffer, LogMode::Default); \
  va_end(args);

void logDebug(const char* format, ...) {
  LOG_PRINTF(Level::Debug);
}

void logInfo(const char* format, ...) {
  LOG_PRINTF(Level::Info);
}

void logWarn(const char* format, ...) {
  LOG_PRINTF(Level::Warn);
}

void logError(const char* format, ...) {
  LOG_PRINTF(Level::Error);
}
