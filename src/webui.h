// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <map>
#include "drum_kit.h"
#include "ble_client.h"

#include <Arduino.h>

#if defined(ARDUINO_ARCH_RP2040)
#include <AsyncWebServer_RP2040W.h>
#else
#include <ESPAsyncWebServer.h>
#endif

struct BleDeviceInfo {
  String name;
  String bdaddr;
};

class WebUI {
public:
  void setup(DrumKit& drumKit);

  void sendBinaryToWebSocket(uint8_t* messageBuffer, size_t size);
  void sendJsonToWebSocket(JsonDocument json, AsyncWebSocketClient* client = nullptr);

  void sendBleScanResult(const std::vector<BleDeviceInfo>& results);
  void sendBleStatus(BleClientStatus status, bool isScanning, AsyncWebSocketClient* client = nullptr);
  void sendUsbHostStatus(const String& deviceName, AsyncWebSocketClient* client = nullptr);

private:
  void initHttpServer();
  void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len);

  void handleCommand(String cmd, JsonObject& argsNode, AsyncWebSocketClient* client);
  
  void handleSetConfigRequest(AsyncWebSocketClient* client, JsonObjectConst configNode);
  void handleSetSettingsRequest(AsyncWebSocketClient* client, JsonObjectConst configNode);
  void handleSetMappingsRequest(JsonObject mappingsNode);
  void handleSetGeneralConfigRequest(JsonObjectConst generalConfigNode);
  void handleSetMonitor(JsonObjectConst configNode);
  void handleSetPadConfig(JsonObjectConst configNode, AsyncWebSocketClient* client);
  void handleTriggerMonitor();
  void handleSaveConfigRequest(AsyncWebSocketClient* client);
  void handleRestoreConfigRequest(AsyncWebSocketClient* client);
  void handleGetConfigRequest(AsyncWebSocketClient* client);
  void handlePlayNote(JsonObjectConst argsNode);
  void handleEventLogRequest(AsyncWebSocketClient* client);
  void handleLatencyTestRequest(JsonObjectConst argsNode, AsyncWebSocketClient* client);
  void handleStatsRequest(AsyncWebSocketClient* client);
  void handleScanBleDevicesRequest(AsyncWebSocketClient* client);
  void handleSetBlePairingRequest(JsonObjectConst argsNode, AsyncWebSocketClient* client);
  void handleGetBleStatusRequest(AsyncWebSocketClient* client);
  void handleGetUsbHostStatusRequest(AsyncWebSocketClient* client);

  void sendConfig(AsyncWebSocketClient* client);
  
  void setMonitorConfig(JsonObject& infoNode);
  void setVersionInfo(JsonObject& infoNode);
  void setAvailableMidiOutputModes(JsonObject& infoNode);

  void sendTextToWebSocket(const String& text);

private:
  DrumKit* drumKit = nullptr;

  bool isConfigDirty = false;

  std::map<int, AsyncWebSocketClient*> connections;
  std::map<int, String> pendingWsTextByClient;
};

extern WebUI webUI;
