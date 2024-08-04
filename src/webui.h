// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>
#include "drum_kit.h"

#include <Arduino.h>

#if defined(ARDUINO_ARCH_RP2040)
#include <AsyncWebServer_RP2040W.h>
#else
#include <ESPAsyncWebServer.h>
#endif

class WebUI {
public:
  void setup(DrumKit& drumKit);

  void sendBinaryToWebSocket(uint8_t* messageBuffer, size_t size);
  void sendTextToWebSocket(const String& text);

private:
  void serve();
  void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

  void handleCommand(String cmd, JsonObjectConst& argsNode, AsyncWebSocketClient* client);
  
  void handleSetSettingsRequest(AsyncWebSocketClient* client, JsonObjectConst configNode);
  void handleSetMappingsRequest(JsonObjectConst mappingsNode);
  void handleSetGeneralConfigRequest(JsonObjectConst generalConfigNode);
  void handleSetMonitor(JsonObjectConst configNode);
  void handleSetPadConfig(JsonObjectConst configNode);
  void handleTriggerMonitor();
  void handleSaveConfigRequest(AsyncWebSocketClient* client);
  void handleRestoreConfigRequest(AsyncWebSocketClient* client);
  void handleGetConfigRequest(AsyncWebSocketClient* client);
  void handlePlayNote(JsonObjectConst& argsNode);
  void handleEventLogRequest(AsyncWebSocketClient* client);
  void handleLatencyTestRequest(JsonObjectConst& argsNode, AsyncWebSocketClient* client);
  void handleStatsRequest(AsyncWebSocketClient* client);

  void sendConfig(AsyncWebSocketClient* client);

private:
  DrumKit* drumKit = nullptr;

  bool isConfigDirty = false;

  std::map<int, AsyncWebSocketClient*> connections;
};

extern WebUI webUI;
