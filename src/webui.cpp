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

void WebUI::sendConfig(AsyncWebSocketClient* client) {
  JsonDocument configDoc = DrumConfigMapper::getDrumKitConfigAsJson(*drumKit);
  JsonObject configNode = configDoc.as<JsonObject>();

  const DrumMonitor& monitor = drumKit->getMonitor();
  const DrumPad* monitoredPad = monitor.getMonitoredPad();
  const JsonObject monitorNode = configNode[CONFIG_MONITOR].to<JsonObject>();
  monitorNode[CONFIG_MONITOR_TRIGGERED_BY_ALL_PADS] = monitor.isTriggeredByAllPads();
  if (monitoredPad) {
    monitorNode[CONFIG_MONITOR_PAD] = monitoredPad->getIndex();
  }
  configNode["latencyTest"] = monitor.isLatencyTestActive();
  configNode["isDirty"] = isConfigDirty;
  
  JsonObject versionNode = configNode["version"].to<JsonObject>();
  versionNode["packageVersion"] = Version::getPackageVersion();
  versionNode["gitCommitHash"] = Version::getGitCommitHash();
  versionNode["buildTime"] = Version::getBuildTime();

  JsonDocument doc;
  doc["config"] = configNode;
  sendJson(doc, client);
}

void WebUI::handleGetConfigRequest(AsyncWebSocketClient* client) {
  sendConfig(client);
}

void WebUI::handleSetMonitor(JsonObjectConst configNode) {
  DrumMonitor& monitor = drumKit->getMonitor();

  if (!configNode[CONFIG_MONITOR_PAD].isUnbound()) {
    DrumPad* monitorPad = nullptr;
    if (configNode[CONFIG_MONITOR_PAD].is<pad_size_t>()) { // unset with null
      pad_size_t padIndex = configNode[CONFIG_MONITOR_PAD];
      if (padIndex < drumKit->getPadsCount()) {
        monitorPad = &drumKit->getPad(padIndex);
      }
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

void WebUI::handleSetPadConfig(JsonObjectConst configNode) {
  for (JsonPairConst pair : configNode) {
    pad_size_t padIndex = atoi(pair.key().c_str());
    if (padIndex >= drumKit->getPadsCount()) {
      SerialDebug.printf("Invalid enable pad: %d\n", padIndex);
      continue;
    }

    if (!pair.value()[CONFIG_ENABLED_PROP].isNull()) {
      bool enabled = pair.value()[CONFIG_ENABLED_PROP];
      drumKit->getPad(padIndex).setEnabled(enabled);
      isConfigDirty = true;
    }

    if (!pair.value()[CONFIG_AUTOCALIBRATE_PROP].isNull()) {
      bool autoCalibrate = pair.value()[CONFIG_AUTOCALIBRATE_PROP];
      drumKit->getPad(padIndex).setAutoCalibrate(autoCalibrate);
      isConfigDirty = true;
    }
  }
}

void WebUI::handleTriggerMonitor() {
  drumKit->getMonitor().triggerMonitor();
}

void WebUI::handleSetSettingsRequest(AsyncWebSocketClient* client, JsonObjectConst settingsNode) {
  DrumConfigMapper::applyDrumKitSettings(*drumKit, settingsNode);
  isConfigDirty = true;
}

void WebUI::handleSetMappingsRequest(JsonObjectConst mappingsNode) {
  DrumConfigMapper::applyDrumKitMappings(*drumKit, mappingsNode);
  isConfigDirty = true;
}

void WebUI::handleSetGeneralConfigRequest(JsonObjectConst generalConfigNode) {
  DrumConfigMapper::applyGeneralConfig(*drumKit, generalConfigNode);
  isConfigDirty = true;
}

void WebUI::handlePlayNote(JsonObjectConst& argsNode) {
  midi_note_t midiNote = argsNode["note"];
  MIDI.sendNoteOn(midiNote, 100, 10);
  MIDI.sendNoteOff(midiNote, 0, 10);
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
    handleSetPadConfig(argsNode);
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
      SerialDebug.printf("Deserialization failed: %s\n", error.c_str());
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
    SerialDebug.println(F("Failed to mount filesystem, check if filesystem content was uploaded"));
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
