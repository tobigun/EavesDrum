// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <functional>
#include "drum.h"

#define MAX_PAD_COUNT 20
#define MAX_MAPPINGS_COUNT 32
#define MAX_MUX_COUNT 4
#define MAX_CONNECTOR_COUNT 32

#define GENERAL_SECTION "general"
#define MUX_SECTION "mux"
#define CONNECTORS_SECTION "connectors"
#define PADS_SECTION "pads"

#define ENUM_TO_STRING(v) (#v)
#define MAP_ENUM_TO_STRING(v) \
  if (value == v) \
    return ENUM_TO_STRING(v);

#define MAP_STRING_TO_ENUM(v) \
  if (value == ENUM_TO_STRING(v)) \
    return v;
  
#if CFG_TUD_MSC
void enableMassStorageDevice();
#endif

class DrumKit;

class DrumConfigMapper {
public:
  DrumConfigMapper() = delete;

public:
  static void saveDrumKitConfig(const DrumKit& drumKit);
  static void loadAndApplyDrumKitConfig(DrumKit& drumKit);

  static JsonDocument getDrumKitConfigAsJson(const DrumKit& drumKit);
  
  static void applyDrumKitMappings(DrumKit& drumKit, JsonObjectConst mappingsNode, bool replace);
  static void applyGeneralConfig(DrumKit& drumKit, JsonObjectConst generalNode);
  static void applyPadSettings(DrumPad& pad, JsonObjectConst settingsNode);

private:
  // From JSON

  static void applyDrumKitConfig(DrumKit& drumKit, const JsonDocument& configNode);

  static void addPadsToDrumKit(DrumKit& drumKit, JsonArrayConst& padsNode);
  static void applyPadConfig(DrumPad& pad, DrumKit& drumKit, pad_size_t padIndex, JsonArrayConst& padsNode);
  static pad_size_t findPedalIndexByName(String pedalRole, JsonArrayConst& padsNode);

  static void addMultiplexersToKit(DrumKit& drumKit, JsonArrayConst& muxNodes);
  static bool applyMuxConfig(DrumMux& mux, JsonObjectConst& muxNode);

  static void addConnectorsToDrumKit(DrumKit& drumKit, JsonObjectConst connectorsNode);
  static bool applyPinsConfig(DrumConnector& connector, DrumKit& drumKit, JsonArrayConst pinsNode);
  static DrumPin getPinsConfig(DrumConnector& connector, DrumKit& drumKit, JsonVariantConst pinsNode);

  static void applyMappings(DrumMappings& mappings, JsonObjectConst& mappingsNode);

  // To JSON
  static void convertGeneralConfigToJson(const DrumKit& drumKit, JsonDocument& config);
  static void convertPadConfigToJson(const DrumPad& pad, const DrumKit& drumKit, JsonObject& padConfigNode);
  static void convertMuxConfigsToJson(const DrumKit& drumKit, JsonDocument& muxNodes);
  static void convertMuxConfigToJson(const DrumMux& mux, const DrumKit& drumKit, JsonObject& muxNode);
  static void convertConnectorConfigsToJson(const DrumKit& drumKit, JsonDocument& config);
  static void convertConnectorConfigToJson(const DrumConnector& pad, const DrumKit& drumKit, JsonObject& connectorNode);
  static void convertPinsToJson(const DrumConnector& pad, JsonArray pinsNode);
  static void convertPadSettingsToJson(const DrumPad& pad, const DrumKit& drumKit, JsonObject& settingsNode);
  static void convertMappingsToJson(const DrumMappings& mappings, JsonObject& mappingsNode);

  // Low level
  static JsonDocument loadDrumKitConfig();
  static void writeDrumKitConfig(JsonDocument doc);
};
