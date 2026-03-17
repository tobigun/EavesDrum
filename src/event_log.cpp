// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "event_log.h"
#include <algorithm>

EventLog eventLog(50);

void EventLog::log(Level level, String message) {
  logString(level, message);

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
