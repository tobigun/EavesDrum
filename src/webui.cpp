// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webui.h"

#include "config/config_mapper.h"
#include "drum_kit.h"
#include "event_log.h"
#include "log.h"
#include "midi/midi_transport_tiny_usb_host.h"
#include "midi_transport.h"
#include "monitor.h"
#include "version.h"
#if HAS_BLUETOOTH
#include "ble_client.h"
#endif

#include <ArduinoJson.h>
#include <LittleFS.h>

#define CONFIG_INFO "_info"

#define CONFIG_INFO_MONITOR "monitor"
#define CONFIG_INFO_MONITOR_PAD "padIndex"
#define CONFIG_INFO_MONITOR_TRIGGERED_BY_ALL_PADS "triggeredByAllPads"

#define CONFIG_MAPPINGS_REPLACE_PROP "_replace"

#define CONFIG_NAME_PROP "name"
#define CONFIG_ROLE_PROP "role"
#define CONFIG_CONNECTOR_PROP "connector"
#define CONFIG_TOUCH_SENSOR_PROP "touchSensor"
#define CONFIG_ENABLED_PROP "enabled"
#define CONFIG_AUTOCALIBRATE_PROP "autoCalibrate"

WebUI webUI;

static AsyncWebServer* server;
static AsyncWebSocket* ws;

void WebUI::handleGetConfigRequest(AsyncWebSocketClient* client) {
  sendConfig(client);
}

void WebUI::sendConfig(AsyncWebSocketClient* client) {
  JsonDocument configDoc = DrumConfigMapper::getDrumKitConfigAsJson(*drumKit);
  JsonObject configNode = configDoc.as<JsonObject>();

  JsonObject infoNode = configDoc[CONFIG_INFO].to<JsonObject>();
  setMonitorConfig(infoNode);
  setVersionInfo(infoNode);
  setAvailableMidiOutputModes(infoNode);

  if (isConfigDirty) {
    infoNode["isDirty"] = isConfigDirty;
  }

  JsonDocument doc;
  doc["config"] = configNode;
  sendJsonToWebSocket(doc, client);
}

void WebUI::setMonitorConfig(JsonObject& infoNode) {
  const DrumMonitor& monitor = drumKit->getMonitor();
  const DrumPad* monitoredPad = monitor.getMonitoredPad();
  bool triggeredByAllPads = monitor.isTriggeredByAllPads();
  bool latencyTestActive = monitor.isLatencyTestActive();

  bool monitorSectionPresent = monitoredPad || triggeredByAllPads || latencyTestActive;
  if (!monitorSectionPresent) {
    return;
  }

  JsonObject monitorNode = infoNode[CONFIG_INFO_MONITOR].to<JsonObject>();
  if (monitoredPad) {
    monitorNode[CONFIG_INFO_MONITOR_PAD] = monitoredPad->getIndex();
  }
  if (triggeredByAllPads) {
    monitorNode[CONFIG_INFO_MONITOR_TRIGGERED_BY_ALL_PADS] = triggeredByAllPads;
  }
  if (latencyTestActive) {
    monitorNode["latencyTest"] = monitor.isLatencyTestActive();
  }
}

void WebUI::setVersionInfo(JsonObject& infoNode) {
  JsonObject versionNode = infoNode["version"].to<JsonObject>();
  versionNode["packageVersion"] = Version::getPackageVersion();
  versionNode["gitCommitHash"] = Version::getGitCommitHash();
  versionNode["buildTime"] = Version::getBuildTime();
}

void WebUI::setAvailableMidiOutputModes(JsonObject& infoNode) {
  JsonArray midiOutputsNode = infoNode["midiOutputModes"].to<JsonArray>();
  for (MidiOutputMode mode : midiTransport.getSupportedOutputModes()) {
    midiOutputsNode.add(midiOutputModeToString(mode));
  }
}

void WebUI::handleSetMonitor(JsonObjectConst configNode) {
  DrumMonitor& monitor = drumKit->getMonitor();

  if (!configNode[CONFIG_INFO_MONITOR_PAD].isUnbound()) {
    DrumPad* monitorPad = nullptr;
    if (configNode[CONFIG_INFO_MONITOR_PAD].is<pad_size_t>()) { // unset with null
      pad_size_t padIndex = configNode[CONFIG_INFO_MONITOR_PAD];
      monitorPad = drumKit->getPad(padIndex);
    }

    if (!monitorPad) {
      monitor.disableMonitor();
      logDebug("Monitor: disabled\n");
    } else {
      monitor.setMonitoredPad(monitorPad);
      logDebug("Monitor: %s\n", monitorPad->getName().c_str());
    }
  }

  if (configNode[CONFIG_INFO_MONITOR_TRIGGERED_BY_ALL_PADS].is<bool>()) {
    bool triggeredByAllPads = configNode[CONFIG_INFO_MONITOR_TRIGGERED_BY_ALL_PADS];
    monitor.setTriggeredByAllPads(triggeredByAllPads);
  }
}

void WebUI::handleSetPadConfig(JsonObjectConst configNode, AsyncWebSocketClient* client) {
  bool sendConfigRequired = false;

  for (JsonPairConst pair : configNode) {
    pad_size_t padIndex = atoi(pair.key().c_str());
    if (padIndex >= drumKit->getPadsCount()) {
      eventLog.log(Level::Error, String("Invalid enable pad: ") + padIndex);
      continue;
    }

    DrumPad& pad = *drumKit->getPad(padIndex);
    JsonVariantConst nodeValue = pair.value();

    if (!nodeValue[CONFIG_CONNECTOR_PROP].isUnbound()) {
      DrumConnector* connector = nullptr;
      if (!nodeValue[CONFIG_CONNECTOR_PROP].isNull()) {
        String connectorId = nodeValue[CONFIG_CONNECTOR_PROP];
        connector = drumKit->getConnectorById(connectorId);
        if (!connector) {
          eventLog.log(Level::Error, String("Disable as connectorId is invalid: ") + connectorId);
        }
      }
      pad.setConnector(connector);
      isConfigDirty = true;
      sendConfigRequired = true;
    }

    if (!nodeValue[CONFIG_TOUCH_SENSOR_PROP].isUnbound()) {
      DrumConnector* touchSensor = nullptr;
      if (!nodeValue[CONFIG_TOUCH_SENSOR_PROP].isNull()) {
        String touchSensorId = nodeValue[CONFIG_TOUCH_SENSOR_PROP];
        touchSensor = drumKit->getConnectorById(touchSensorId);
        if (!touchSensor) {
          eventLog.log(Level::Error, String("Disable as touchSensorId is invalid: ") + touchSensorId);
        }
      }
      pad.setTouchSensor(touchSensor);
      isConfigDirty = true;
      sendConfigRequired = true;
    }

    if (nodeValue[CONFIG_NAME_PROP].is<String>()) {
      String name = nodeValue[CONFIG_NAME_PROP];
      pad.setName(name);
      isConfigDirty = true;
      sendConfigRequired = true;
    }

    if (nodeValue[CONFIG_ROLE_PROP].is<String>()) {
      String role = nodeValue[CONFIG_ROLE_PROP];
      pad.setRole(role);
      pad.setMappings(drumKit->getMappings(role));
      isConfigDirty = true;
      sendConfigRequired = true;
    }

    if (nodeValue[CONFIG_ENABLED_PROP].is<bool>()) {
      bool enabled = nodeValue[CONFIG_ENABLED_PROP];
      pad.setEnabled(enabled);
      isConfigDirty = true;
    }

    if (nodeValue[CONFIG_AUTOCALIBRATE_PROP].is<bool>()) {
      bool autoCalibrate = nodeValue[CONFIG_AUTOCALIBRATE_PROP];
      pad.setAutoCalibrate(autoCalibrate);
      isConfigDirty = true;
    }
  }

  if (sendConfigRequired) {
    sendConfig(client);
  }
}

void WebUI::handleSetSettingsRequest(AsyncWebSocketClient* client, JsonObjectConst settingsNode) {
  for (JsonPairConst keyValuePair : settingsNode) {
    pad_size_t padIndex = atoi(keyValuePair.key().c_str());
    DrumPad* pad = drumKit->getPad(padIndex);
    if (!pad) {
      eventLog.log(Level::Error, String("Invalid pad in setSettings request: ") + padIndex);
      continue;
    }
    DrumConfigMapper::applyPadSettings(*pad, keyValuePair.value());
    isConfigDirty = true;
  }
}

void WebUI::handleSetConfigRequest(AsyncWebSocketClient* client, JsonObjectConst configNode) {
  JsonDocument configDoc = DrumConfigMapper::getDrumKitConfigAsJson(*drumKit);
  for (JsonPairConst keyValuePair : configNode) {
    configDoc[keyValuePair.key()] = keyValuePair.value();
  }

  const bool writeSuccess = DrumConfigMapper::writeDrumKitConfig(configDoc);
  isConfigDirty = false;

  JsonDocument resultDoc;
  JsonObject resultNode = resultDoc["setConfigResult"].to<JsonObject>();
  resultNode["success"] = writeSuccess;
  if (!writeSuccess) {
    resultNode["message"] = "Could not write config file";
    sendJsonToWebSocket(resultDoc, client);
    return;
  }

  sendJsonToWebSocket(resultDoc, client);

  logInfo("Config applied. Reset required ...\n");
  if (!DrumIO::requestReset(1000)) { // wait a bit until the result was sent
    // only send new config if reset was not successful (e.g. on PC), as otherwise the client will request the config again after reconnecting
    sendConfig(client);
  }
}

void WebUI::handleSetMappingsRequest(JsonObject mappingsNode) {
  // if the replace property is set, replace existing mappings instead of merging them
  bool replace = false;
  if (mappingsNode[CONFIG_MAPPINGS_REPLACE_PROP].is<bool>()) { // remove UI specific property
    replace = mappingsNode[CONFIG_MAPPINGS_REPLACE_PROP].as<bool>();
    mappingsNode.remove(CONFIG_MAPPINGS_REPLACE_PROP);
  }

  // if there is more than one mapping we assume that the user wants to replace all mappings (e.g. EZDrummer by Addictive Drums).
  // We have to delete all existing mappings first as not all old roles might be defined in the new config
  bool replaceAll = replace && (mappingsNode.size() - 1) > 1;
  if (replaceAll) {
    drumKit->deleteAllMappings();
    replace = false; // no need to replace individual mappings as we already deleted everything
  }

  DrumConfigMapper::applyDrumKitMappings(*drumKit, mappingsNode, replace);
  isConfigDirty = true;
}

void WebUI::handleSetGeneralConfigRequest(JsonObjectConst generalConfigNode) {
  DrumConfigMapper::applyGeneralConfig(*drumKit, generalConfigNode);
  isConfigDirty = true;
}

void WebUI::handleTriggerMonitor() {
  drumKit->getMonitor().triggerMonitor();
}

void WebUI::handlePlayNote(JsonObjectConst argsNode) {
  midi_note_t midiNote = argsNode["note"];
  drumKit->sendMidiNoteOnOffMessage(midiNote, 100);
}

void WebUI::handleEventLogRequest(AsyncWebSocketClient* client) {
  JsonDocument doc;
  JsonArray eventsNode = doc["events"].to<JsonArray>();
  eventLog.forEach([&eventsNode](int id, Level level, String message) {
    JsonObject eventNode = eventsNode.add<JsonObject>();
    eventNode["id"] = id;
    eventNode["level"] = (int)level;
    eventNode["message"] = message;
  });
  sendJsonToWebSocket(doc, client);
}

void WebUI::handleStatsRequest(AsyncWebSocketClient* client) {
  JsonDocument doc;
  JsonObject statsNode = doc["stats"].to<JsonObject>();

  uint32_t updateCountPer30s = drumKit->statistics.updateCountPer30s;
  if (updateCountPer30s != 0) {
    statsNode["updateCountPer30s"] = updateCountPer30s;
  } else {
    statsNode["updateCountPer30s"] = nullptr;
  }

  sendJsonToWebSocket(doc, client);
}

void WebUI::handleSaveConfigRequest(AsyncWebSocketClient* client) {
  DrumConfigMapper::saveDrumKitConfig(*drumKit);
  isConfigDirty = false;

  sendConfig(client);
}

void WebUI::handleRestoreConfigRequest(AsyncWebSocketClient* client) {
  if (!DrumIO::requestReset()) {
    isConfigDirty = false;
    sendConfig(client);
  }
}

void WebUI::handleLatencyTestRequest(JsonObjectConst argsNode, AsyncWebSocketClient* client) {
  DrumMonitor& monitor = drumKit->getMonitor();
  bool enabled = argsNode["enabled"];
  if (enabled) {
    bool preview = argsNode["preview"];
    sensor_value_t threshold = argsNode["threshold"];
    midi_note_t midiNote = argsNode["midiNote"] | 38;
    monitor.startLatencyTest(preview, threshold, midiNote);
    if (preview) {
      // send config as the latency test might not be started if no monitored pad was selected
      // but only send it in preview mode to not interfer with the test
      sendConfig(client);
    }
  } else {
    monitor.stopLatencyTest();
    sendConfig(client);
  }
}

void WebUI::handleScanBleDevicesRequest(AsyncWebSocketClient* client) {
#if HAS_BLUETOOTH
  bleClient.startDeviceScan();
#endif
}

void WebUI::handleSetBlePairingRequest(JsonObjectConst argsNode, AsyncWebSocketClient* client) {
#if HAS_BLUETOOTH
  String name = argsNode["name"] | "";
  String address = argsNode["address"] | "";
  bleClient.setPairingInfo(name, address);

  isConfigDirty = true;
  sendConfig(client);
#endif
}

void WebUI::handleGetBleStatusRequest(AsyncWebSocketClient* client) {
#if HAS_BLUETOOTH
  BleClientStatus status = bleClient.getStatus();
  bool isScanning = bleClient.isScanning();
  sendBleStatus(status, isScanning, client);
#endif
}

void WebUI::sendBleScanResult(const std::vector<BleDeviceInfo>& results) {
  JsonDocument doc;
  JsonArray devicesNode = doc["bleDevices"].to<JsonArray>();
  for (const BleDeviceInfo& device : results) {
    JsonObject deviceNode = devicesNode.add<JsonObject>();
    deviceNode["name"] = device.name;
    deviceNode["address"] = device.bdaddr;
  }
  sendJsonToWebSocket(doc);
}

void WebUI::sendBleStatus(BleClientStatus status, bool isScanning, AsyncWebSocketClient* client) {
  JsonDocument doc;
  JsonObject devicesNode = doc["bleStatus"].to<JsonObject>();

  switch (status) {
  case BleClientStatus::Disconnected:
    devicesNode["status"] = "disconnected";
    break;
  case BleClientStatus::Connecting:
    devicesNode["status"] = "connecting";
    break;
  case BleClientStatus::Connected:
    devicesNode["status"] = "connected";
    break;
  default:
    devicesNode["status"] = "unknown";
    break;
  }

  devicesNode["scanning"] = isScanning;

  sendJsonToWebSocket(doc, client);
}

void WebUI::handleGetUsbHostStatusRequest(AsyncWebSocketClient* client) {
#ifdef ENABLE_MIDI_TINY_USB_HOST_TRANSPORT
  sendUsbHostStatus(midiTransportTinyUsbHost.getConnectedDeviceName(), client);
#endif
}

void WebUI::sendUsbHostStatus(const String& deviceName, AsyncWebSocketClient* client) {
  JsonDocument doc;
  JsonObject statusNode = doc["usbHostStatus"].to<JsonObject>();
  if (deviceName.length() > 0) {
    statusNode["deviceName"] = deviceName;
  } else {
    statusNode["deviceName"] = nullptr;
  }

  sendJsonToWebSocket(doc, client);
}

void WebUI::handleCommand(String cmd, JsonObject& argsNode, AsyncWebSocketClient* client) {
  if (cmd == "getConfig") {
    handleGetConfigRequest(client);
  } else if (cmd == "setConfig") {
    handleSetConfigRequest(client, argsNode);
  } else if (cmd == "setSettings") {
    handleSetSettingsRequest(client, argsNode);
  } else if (cmd == "setMappings") {
    handleSetMappingsRequest(argsNode);
  } else if (cmd == "setGeneral") {
    handleSetGeneralConfigRequest(argsNode);
  } else if (cmd == "setMonitor") {
    handleSetMonitor(argsNode);
  } else if (cmd == "setPadConfig") {
    handleSetPadConfig(argsNode, client);
  } else if (cmd == "triggerMonitor") {
    handleTriggerMonitor();
  } else if (cmd == "saveConfig") {
    handleSaveConfigRequest(client);
  } else if (cmd == "restoreConfig") {
    handleRestoreConfigRequest(client);
  } else if (cmd == "playNote") {
    handlePlayNote(argsNode);
  } else if (cmd == "latencyTest") {
    handleLatencyTestRequest(argsNode, client);
  } else if (cmd == "getEvents") {
    handleEventLogRequest(client);
  } else if (cmd == "getStats") {
    handleStatsRequest(client);
  } else if (cmd == "scanBleDevices") {
    handleScanBleDevicesRequest(client);
  } else if (cmd == "blePair") {
    handleSetBlePairingRequest(argsNode, client);
  } else if (cmd == "getBleStatus") {
    handleGetBleStatusRequest(client);
  } else if (cmd == "getUsbHostStatus") {
    handleGetUsbHostStatusRequest(client);
  } else {
    logInfo("Cmd: %s", cmd.c_str());
  }
}

void WebUI::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
  case WS_EVT_CONNECT: {
    logInfo("WebSocket client connected: %d", client->id());
    client->setCloseClientOnQueueFull(false);
    client->ping();
    break;
  }

  case WS_EVT_DISCONNECT: {
    pendingWsTextByClient.erase(client->id());
    logInfo("WebSocket client disconnected: %d", client->id());
    break;
  }

  case WS_EVT_DATA: {
    AwsFrameInfo* frameInfo = (AwsFrameInfo*)arg;
    if (!frameInfo) {
      logError("Received WebSocket message without frame info");
      return;
    }

    logDebug("index: %" PRIu64 ", len: %" PRIu64 ", final: %" PRIu8 ", opcode: %" PRIu8 ", framelen: %d\n",
      frameInfo->index, frameInfo->len, frameInfo->final, frameInfo->message_opcode, len);

    if (frameInfo->message_opcode != WS_TEXT) {
      logWarn("WebSocket binary message received -> ignore");
      return;
    }

    String frame = String((const char *)data, len);

    bool isCompleteFrame = frameInfo->final && frameInfo->index == 0 && frameInfo->len == len;
    if (isCompleteFrame) { // data contains complete frame
      client->ping();
      handleTextMessage(client, frame);
      pendingWsTextByClient.erase(client->id());
      return;
    } else { // data contains fragment of message
      bool hasPreviousFragments = pendingWsTextByClient.contains(client->id());

      bool isMessageStart = frameInfo->index == 0 && frameInfo->num == 0;
      if (isMessageStart) {
        if (hasPreviousFragments) {
          logError("Received fragment with index=0 but previous fragments exist -> reset buffer");
        }
        pendingWsTextByClient[client->id()] = frame;
      } else {
        if (!hasPreviousFragments) {
          logError("Received WebSocket message fragment but no previous fragments exist -> ignore");
          return;
        }
        pendingWsTextByClient[client->id()] += frame;
      }

      // check for message end
      bool isMessageEnd = (frameInfo->index + len) == frameInfo->len && frameInfo->final;
      if (isMessageEnd) {
        String& message = pendingWsTextByClient[client->id()];
        handleTextMessage(client, message);
        pendingWsTextByClient.erase(client->id());
      }
    }
    break;
  }

  case WS_EVT_ERROR:
    logError("WebSocket error");
    break;

  case WS_EVT_PONG:
  default:
    break;
  }
}

void WebUI::handleTextMessage(AsyncWebSocketClient* client, const String& message) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    eventLog.log(Level::Error, String("Deserialization failed: ") + error.c_str());
    return;
  }

  for (auto cmdNode : doc.as<JsonObject>()) {
    const String cmd = cmdNode.key().c_str();
    JsonObject cmdArgsNode = cmdNode.value();
    handleCommand(cmd, cmdArgsNode, client);
  }
}

void WebUI::initHttpServer() {
  server = new AsyncWebServer(80);
  ws = new AsyncWebSocket("/ws");

  bool fsBegin = LittleFS.begin();
  if (!fsBegin) {
    eventLog.log(Level::Error, F("Failed to mount filesystem, check if filesystem content was uploaded"));
    return;
  }

  ws->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type,
                  void* arg, uint8_t* data, size_t len) {
    this->onWsEvent(server, client, type, arg, data, len);
  });
  server->addHandler(ws);

  server->serveStatic("/", LittleFS, "/", "max-age=604800")
      .setDefaultFile("index.html");

  server->onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "text/html", "Resource not found. Did you upload the filesystem content?");
  });

  server->begin();

  logInfo("UI Initialized");
}

void WebUI::setup(DrumKit& drumKit) {
  this->drumKit = &drumKit;
  initHttpServer();
}

void WebUI::sendJsonToWebSocket(JsonDocument json, AsyncWebSocketClient* client) {
  String str;
  serializeJson(json, str);

  if (client) {
    client->text(str);
  } else {
    sendTextToWebSocket(str);
  }
}

void WebUI::sendTextToWebSocket(const String& text) {
  ws->textAll(text);
}

void WebUI::sendBinaryToWebSocket(uint8_t* messageBuffer, size_t size) {
  ws->binaryAll(messageBuffer, size);
}
