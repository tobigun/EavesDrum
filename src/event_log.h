// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>
#include <deque>
#include <functional>

enum class Level {
  INFO,
  WARN,
  ERROR
};

struct EventLogEntry {
  int id;
  Level level;
  String message;
};

class EventLog {
private:
  int nextEventId = 0;
  const size_t eventListSize;
  std::deque<EventLogEntry> eventList;

public:
  EventLog(size_t eventListSize)
    : eventListSize(eventListSize) {}

  void log(Level level, const char* message) { log(level, String(message)); }
  void log(Level level, String message);

  void forEach(const std::function<void (int id, Level level, String message)>& func);
};

extern EventLog eventLog;