// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "log.h"
#include "pad_sound_playback.h"
#include "simulation.h"
#include "monitor.h"

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#if __has_include ("conio.h")
#include <conio.h>
#define HAS_CONIO
#endif
#include <unistd.h>
#include <memory>

static const std::map<int, int> shiftedKeys = {
    {'!', 1},
    {'"', 2},
    {245, 3},
    {'$', 4},
    {'%', 5},
    {'&', 6},
    {'/', 7},
    {'(', 8},
    {')', 9},
    {'=', 0}};

enum class CommandAction {
  Hit,
  Choke,
  Increase,
  Decrease,
  Jitter
};

static int jitterLevel = 0;
static sensor_value_t sensorValue = 0;

static std::shared_ptr<PadSoundPlayback> padPlayback;

void setupUsb() {}

void Network::setup() {}
void Network::service_traffic() {
  AsyncWebServer::poll();
}

void updatePlayback() {
  const DrumMonitor& monitor = drumKit.getMonitor();
  if (monitor.isLatencyTestActive() && !padPlayback) {
    padPlayback = std::make_shared<PadSoundPlayback>(monitor.getMonitoredPad(), false);
  }

  if (padPlayback) {
    padPlayback->update();
    if (!padPlayback->isPlaying()) {
      padPlayback.reset();
    }  
  }
}

void updateSignal() {
  const DrumPad* pad = drumKit.getMonitor().getMonitoredPad();
  if (pad && jitterLevel) {
    int jitter = 10 * jitterLevel * (2. * rand() / RAND_MAX - 1);
    sensor_value_t value = min(max((int)sensorValue + jitter, 0), MAX_SENSOR_VALUE);
    setPadPinValue(*pad, 0, value);
  }
}

#ifndef UNIT_TEST

static pad_size_t keyToPadIndex(int key) {
  return key == '0' ? 9 : key - '1';
}

static const DrumPad* getHitPadAndAction(int key, CommandAction& action) {
  action = CommandAction::Hit;

  pad_size_t hitPadIndex = 0;
  if (key >= '0' && key <= '9') {
    hitPadIndex = keyToPadIndex(key);
  } else if (shiftedKeys.find(key) != shiftedKeys.end()) {
    action = CommandAction::Choke;
    int numericKey = shiftedKeys.at(key);
    hitPadIndex = keyToPadIndex(numericKey);
  } else if (drumKit.getMonitor().getMonitoredPad()) {
    if (key == '+') {
      action = CommandAction::Increase;
    } else if (key == '-') {
      action = CommandAction::Decrease;
    } else if (key == '#') {
      action = CommandAction::Jitter;
    }
    return drumKit.getMonitor().getMonitoredPad();
  }

  if (hitPadIndex >= drumKit.getPadsCount()) {
    hitPadIndex = 0;
  }

  return drumKit.getPad(hitPadIndex);
}

static int getPressedKey() {
  #ifdef HAS_CONIO
  if (_kbhit()) {
    return _getch();
  }
  #endif
  return 0;
}

static void handleInput() {
  if (padPlayback) {
    return;
  }

  int key = getPressedKey();
  if (!key) {
    return;
  }

  CommandAction action;
  const DrumPad* hitPad = getHitPadAndAction(key, action);
  if (action == CommandAction::Increase) {
    sensorValue = min((int)sensorValue + 100, MAX_SENSOR_VALUE);
    setPadPinValue(*hitPad, 0, sensorValue);
    SerialDebug.printf("Sensor-Value: %d\n", sensorValue);
  } else if (action == CommandAction::Decrease) {
    sensorValue = max((int)sensorValue - 100, 0);
    setPadPinValue(*hitPad, 0, sensorValue);
    SerialDebug.printf("Sensor-Value: %d\n", sensorValue);
  } else if (action == CommandAction::Jitter) {
    jitterLevel = (jitterLevel + 1) % 5;
    SerialDebug.printf("Jitter-Level: %d\n", jitterLevel);
  } else {
    padPlayback = std::make_shared<PadSoundPlayback>(hitPad, action == CommandAction::Choke);
  }
}

int main() {
  setup();

  while (true) {
    handleInput();
    updatePlayback();
    updateSignal();
    loop();
  }

  return 0;
}

#endif