// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "drum.h"
#include "monitor_history.h"

#include <Arduino.h>

class DrumKit;

enum LatencyTestState {
  INACTIVE,
  WAIT_FOR_HIT,
  SENDING_RESULT
};

typedef uint8_t enum_uint8_t;

struct LatencyTestInfo {
  time_us_t startTimeUs = 0;
  LatencyTestState state = INACTIVE;
  sensor_value_t threshold;
  bool preview; // if not preview: test mode (trigger only once)
};

struct MonitorHitInfo {
  pad_size_t padIndex;
  midi_velocity_t velocities[3];
  history_bool_t hits[3];
  history_bool_t isChoked = false; // bool: not platform independant
  sensor_value_t zoneThresholdsMin[3];
  sensor_value_t zoneThresholdsMax[3];
  history_index_t triggerStartIndex;
  history_index_t triggerEndIndex;
  uint32_t latencyUs;
  enum_uint8_t padType;
  enum_uint8_t zonesType;
  enum_uint8_t chokeType;
} __attribute__((packed));

struct MonitorMessage {
  MonitorHitInfo hitInfo;
  HistoryEntry history[MONITOR_HISTORY_COUNT];
} __attribute__((packed));

class DrumMonitor {
public:
  DrumMonitor(DrumKit* drumKit) : drumKit(drumKit) {}

  // returns true if a hit was sent
  bool checkAndSendMonitoredPadHitInfo();
  void checkAndSendNonMonitoredPadHitInfo();

  void setMonitoredPad(DrumPad& pad) {
    setMonitoredPad(&pad);
  }

  void setMonitoredPad(DrumPad* pad) {
    stopLatencyTest();
    monitoredPad = pad;
  }

  void triggerMonitor() {
    isExternallyTriggered = true;
  }

  void disableMonitor() {
    setMonitoredPad(nullptr);
  }

  /**
   * @return the monitored pad or nullptr if monitoring is disabled.
   */
  DrumPad* getMonitoredPad() {
    return const_cast<DrumPad*>(monitoredPad);
  }
  
  const DrumPad* getMonitoredPad() const {
    return const_cast<const DrumPad*>(monitoredPad);
  }

  const bool isTriggeredByAllPads() const {
    return triggeredByAllPads;
  }
  
  void setTriggeredByAllPads(bool value) {
    triggeredByAllPads = value;
  }

  void sendHitMessage(const MonitorHitInfo& monitorInfo, bool includeHistoryData);

  void startLatencyTest(bool preview, sensor_value_t threshold, midi_note_t midiNote);
  
  void stopLatencyTest() {
    latencyTest.startTimeUs = 0;
    latencyTest.state = INACTIVE;
  }

  void updateLatencyTest(time_us_t senseTimeUs);

  bool isLatencyTestActive() const {
    return latencyTest.state != INACTIVE;
  }

private:
  bool checkAndSendMonitoredDrumHitInfo(const DrumPad& pad);
  bool checkAndSendMonitoredPedalHitInfo(const DrumPad& pedal);
  bool checkAndSendLatencyHitInfo(const DrumPad& pedal);
  bool waitOrSendHitMessage(bool sendIfHistoryFull);

  MonitorHitInfo prepareDrumHitInfo(const DrumPad& pad);
  MonitorHitInfo prepareLatencyTestHitInfo(const DrumPad& pad);
  MonitorHitInfo preparePedalHitInfo(const DrumPad& pedal);
  
  // returns false if the wait time between history entry writes is not yet over
  bool addPadHistoryEntry(const DrumPad& pad);

private:
  DrumKit* drumKit;

  volatile DrumPad* monitoredPad = nullptr;
  bool triggeredByAllPads = false;

  MonitorHitInfo monitoredPadHitInfo;
  time_ms_t nextHitSendTimeMs = 0;

  MonitorHistory history;

  time_us_t lastHistoryUpdateTimeUs = 0;

  MonitorMessage msgBuffer;

  bool isExternallyTriggered = false;

  LatencyTestInfo latencyTest;
};
