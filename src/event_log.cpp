// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "event_log.h"
#include "log.h"
#include <algorithm>

EventLog eventLog(50);

static const char* levelToString(Level level) {
  switch (level) {
    case Level::INFO: return "INFO";
    case Level::ERROR: return "ERROR";
    case Level::WARN: return "WARN";
    default: return "";
  }  
}

void EventLog::log(Level level, String message) {
  SerialDebug.printf("%s: %s\n", levelToString(level), message.c_str());

  EventLogEntry entry = {id : nextEventId++, level : level, message : message};
  eventList.push_back(entry);
  if (eventList.size() == eventListSize) {
    eventList.pop_front();
  }
}

void EventLog::forEach(const std::function<void(int id, Level level, String message)>& func) {
  for (auto event : eventList) {
    func(event.id, event.level, event.message);
  }
}

void logError(String error) {
  eventLog.log(Level::ERROR, error);
}
