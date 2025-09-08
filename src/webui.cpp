// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "webui.h"

#include "config/config_mapper.h"
#include "drum_kit.h"
#include "event_log.h"
#include "log.h"
#include "midi_device.h"
#include "monitor.h"
#include "version.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

#define CONFIG_MONITOR "monitor"
#define CONFIG_MONITOR_PAD "padIndex"
#define CONFIG_MONITOR_TRIGGERED_BY_ALL_PADS "triggeredByAllPads"

#define CONFIG_NAME_PROP "name"
#define CONFIG_ROLE_PROP "role"
#define CONFIG_CONNECTOR_PROP "connector"
#define CONFIG_ENABLED_PROP "enabled"
#define CONFIG_AUTOCALIBRATE_PROP "autoCalibrate"

WebUI webUI;

static AsyncWebServer* server;
static AsyncWebSocket* ws;

static void sendJson(JsonDocument json, AsyncWebSocketClient* client) {
  String str;
  serializeJson(json, str);
  client->text(str);
}

void WebUI::handleGetConfigRequest(AsyncWebSocketClient* client) {
  sendConfig(client);
}

void WebUI::sendConfig(AsyncWebSocketClient* client) {
  JsonDocument configDoc = DrumConfigMapper::getDrumKitConfigAsJson(*drumKit);
  JsonObject configNode = configDoc.as<JsonObject>();

  setMonitorConfig(configNode);
  setVersionInfo(configNode);

  if (isConfigDirty) {
    configNode["isDirty"] = isConfigDirty;
  }
  
  JsonDocument doc;
  doc["config"] = configNode;
  sendJson(doc, client);
}

void WebUI::setMonitorConfig(JsonObject& configNode) {
  const DrumMonitor& monitor = drumKit->getMonitor();
  const DrumPad* monitoredPad = monitor.getMonitoredPad();
  bool triggeredByAllPads = monitor.isTriggeredByAllPads();
  bool latencyTestActive = monitor.isLatencyTestActive();

  bool monitorSectionPresent = monitoredPad || triggeredByAllPads || latencyTestActive;
  if (!monitorSectionPresent) {
    return;
  }

  JsonObject monitorNode = configNode[CONFIG_MONITOR].to<JsonObject>();
  if (monitoredPad) {
    monitorNode[CONFIG_MONITOR_PAD] = monitoredPad->getIndex();
  }
  if (triggeredByAllPads) {
    monitorNode[CONFIG_MONITOR_TRIGGERED_BY_ALL_PADS] = triggeredByAllPads;
  }
  if (latencyTestActive) {
    monitorNode["latencyTest"] = monitor.isLatencyTestActive();
  }
}

void WebUI::setVersionInfo(JsonObject& configNode) {
  JsonObject versionNode = configNode["version"].to<JsonObject>();
  versionNode["packageVersion"] = Version::getPackageVersion();
  versionNode["gitCommitHash"] = Version::getGitCommitHash();
  versionNode["buildTime"] = Version::getBuildTime();
}

void WebUI::handleSetMonitor(JsonObjectConst configNode) {
  DrumMonitor& monitor = drumKit->getMonitor();

  if (!configNode[CONFIG_MONITOR_PAD].isUnbound()) {
    DrumPad* monitorPad = nullptr;
    if (configNode[CONFIG_MONITOR_PAD].is<pad_size_t>()) { // unset with null
      pad_size_t padIndex = configNode[CONFIG_MONITOR_PAD];
      monitorPad = drumKit->getPad(padIndex);
    }

    if (!monitorPad) {
      monitor.disableMonitor();
      SerialDebug.printf("Monitor: disabled\n");
    } else {
      monitor.setMonitoredPad(monitorPad);
      SerialDebug.printf("Monitor: %s\n", monitorPad->getName().c_str());
    }
  }

  if (configNode[CONFIG_MONITOR_TRIGGERED_BY_ALL_PADS].is<bool>()) {
    bool triggeredByAllPads = configNode[CONFIG_MONITOR_TRIGGERED_BY_ALL_PADS];
    monitor.setTriggeredByAllPads(triggeredByAllPads);
  }
}

void WebUI::handleSetPadConfig(JsonObjectConst configNode, AsyncWebSocketClient* client) {
  bool sendConfigRequired = false;

  for (JsonPairConst pair : configNode) {
    pad_size_t padIndex = atoi(pair.key().c_str());
    if (padIndex >= drumKit->getPadsCount()) {
      eventLog.log(Level::ERROR, String("Invalid enable pad: ") + padIndex);
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
          eventLog.log(Level::ERROR, String("Disable as connectorId is invalid: ") + connectorId);
        }
      }
      pad.setConnector(connector);
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
      eventLog.log(Level::ERROR, String("Invalid pad in setSettings request: ") + padIndex);
      continue;
    }
    DrumConfigMapper::applyPadSettings(*pad, keyValuePair.value());
    isConfigDirty = true;
  }
}

void WebUI::handleSetMappingsRequest(JsonObjectConst mappingsNode) {
  DrumConfigMapper::applyDrumKitMappings(*drumKit, mappingsNode);
  isConfigDirty = true;
}

void WebUI::handleSetGeneralConfigRequest(JsonObjectConst generalConfigNode) {
  DrumConfigMapper::applyGeneralConfig(*drumKit, generalConfigNode);
  isConfigDirty = true;
}

void WebUI::handleTriggerMonitor() {
  drumKit->getMonitor().triggerMonitor();
}

void WebUI::handlePlayNote(JsonObjectConst& argsNode) {
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
  sendJson(doc, client);
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

  sendJson(doc, client);
}

void WebUI::handleSaveConfigRequest(AsyncWebSocketClient* client) {
  DrumConfigMapper::saveDrumKitConfig(*drumKit);
  isConfigDirty = false;

  handleGetConfigRequest(client);
}

void WebUI::handleRestoreConfigRequest(AsyncWebSocketClient* client) {
  DrumIO::reset();

  // we reach this only if we performed a soft reset (e.g. on PC)
  isConfigDirty = false;
  handleGetConfigRequest(client);
}

void WebUI::handleLatencyTestRequest(JsonObjectConst& argsNode, AsyncWebSocketClient* client) {
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

void WebUI::handleCommand(String cmd, JsonObjectConst& argsNode, AsyncWebSocketClient* client) {
  if (cmd == "getConfig") {
    handleGetConfigRequest(client);
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
  } else {
    SerialDebug.printf("Cmd: %s\n", cmd.c_str());
  }
}

void WebUI::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
  case WS_EVT_PONG: {
    break;
  }
  case WS_EVT_ERROR: {
    break;
  }
  case WS_EVT_CONNECT: {
    connections[client->id()] = client;
    break;
  }
  case WS_EVT_DISCONNECT: {
    connections.erase(client->id());
  }
  case WS_EVT_DATA: {
    if (data == nullptr || len == 0) {
      return;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data, len);
    if (error) {
      eventLog.log(Level::ERROR, String("Deserialization failed: ") + error.c_str());
      return;
    }

    for (auto cmdNode : doc.as<JsonObjectConst>()) {
      const String cmd = cmdNode.key().c_str();
      JsonObjectConst cmdArgsNode = cmdNode.value();
      handleCommand(cmd, cmdArgsNode, client);
    }

    break;
  }
  default: {
    break;
  }
  }
}

void WebUI::serve() {
  server = new AsyncWebServer(80);
  ws = new AsyncWebSocket("/ws");

  bool fsBegin = LittleFS.begin();
  if (!fsBegin) {
    eventLog.log(Level::ERROR, F("Failed to mount filesystem, check if filesystem content was uploaded"));
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
    request->send(404);
  });

  server->begin();

  SerialDebug.println(F("UI Initialized"));
}

void WebUI::setup(DrumKit& drumKit) {
  this->drumKit = &drumKit;
  serve();
}

void WebUI::sendTextToWebSocket(const String& text) {
  for (auto connection : connections) {
    AsyncWebSocketClient* client = connection.second;
    if (client->canSend()) {
      client->text(text);
    }  
  }
}

void WebUI::sendBinaryToWebSocket(uint8_t* messageBuffer, size_t size) {
  for (auto connection : connections) {
    AsyncWebSocketClient* client = connection.second;
    if (client->canSend()) {
      client->binary(messageBuffer, size);
    }
  }
}
