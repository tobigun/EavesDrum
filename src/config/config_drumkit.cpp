// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"
#include "drum_kit.h"

#define MAPPINGS_SECTION "mappings"

#define GENERAL_GATETIME "gateTimeMs"

///////////////////////////// From JSON

void DrumConfigMapper::loadAndApplyDrumKitConfig(DrumKit& drumKit) {
  JsonDocument configNode = loadDrumKitConfig();
  if (!configNode.isNull()) {
    applyDrumKitConfig(drumKit, configNode);
  }
}

void DrumConfigMapper::applyDrumKitConfig(DrumKit& drumKit, const JsonDocument& configNode) {
  JsonObjectConst generalNode = configNode[GENERAL_SECTION];
  applyGeneralConfig(drumKit, generalNode);

  JsonArrayConst muxNodes = configNode[MUX_SECTION];
  applyMuxConfigs(drumKit, muxNodes);

  JsonObjectConst connectorsNodes = configNode[CONNECTORS_SECTION];
  applyConnectorsConfig(drumKit, connectorsNodes);
  
  JsonArrayConst padsNode = configNode[PADS_SECTION];
  applyPadConfigs(drumKit, padsNode);

  JsonObjectConst mappingsNode = configNode[MAPPINGS_SECTION];
  applyDrumKitMappings(drumKit, mappingsNode);

  drumKit.init();

  eventLog.log(Level::INFO, String("Config: #multiplexers: ") + drumKit.getMuxCount()
    + ", #pads: " + drumKit.getPadsCount()
    + ", #connectors: " + drumKit.getConnectorsCount()
  );
}

void DrumConfigMapper::applyGeneralConfig(DrumKit& drumKit, JsonObjectConst generalNode) {
  if (!generalNode) {
    eventLog.log(Level::INFO, "Config: " GENERAL_SECTION " node missing");
    return;
  }

  if (generalNode[GENERAL_GATETIME].is<int>()) {
    drumKit.setGateTime(generalNode[GENERAL_GATETIME].as<int>());
  }
}

void DrumConfigMapper::applyDrumKitMappings(DrumKit& drumKit, JsonObjectConst mappingsNode) {
  if (!mappingsNode) {
    eventLog.log(Level::ERROR, "Config: mappings node missing");
    return;
  }

  for (JsonPairConst padKeyValuePair : mappingsNode) {
    String role = String(padKeyValuePair.key().c_str());
    DrumPad* pad = drumKit.getPadByRole(role);
    if (!pad) {
      eventLog.log(Level::ERROR, String("Config: mappings reference pad with role ") + role + " which does not exist");
      continue;
    }

    JsonObjectConst padValuesNode = padKeyValuePair.value();
    DrumConfigMapper::applyPadMappings(*pad, padValuesNode);
  }
}

///////////////////////////// To JSON

void DrumConfigMapper::saveDrumKitConfig(const DrumKit& drumKit) {
  JsonDocument doc = getDrumKitConfigAsJson(drumKit);
  writeDrumKitConfig(doc);
}

JsonDocument DrumConfigMapper::getDrumKitConfigAsJson(const DrumKit& drumKit) {
  JsonDocument config;

  convertGeneralConfigToJson(drumKit, config);
  convertMuxConfigsToJson(drumKit, config);
  convertConnectorConfigsToJson(drumKit, config);

  JsonArray padsNode = config[PADS_SECTION].to<JsonArray>();
  JsonObject mappingsNode = config[MAPPINGS_SECTION].to<JsonObject>();

  for (pad_size_t padIndex = 0; padIndex < drumKit.getPadsCount(); ++padIndex) {
    const DrumPad& pad = *drumKit.getPad(padIndex);

    JsonObject padConfigNode = padsNode[padIndex].to<JsonObject>();
    convertPadConfigToJson(pad, drumKit, padConfigNode);

    JsonObject padMappingsNode = mappingsNode[pad.getRole()].to<JsonObject>();
    convertPadMappingsToJson(pad, drumKit, padMappingsNode);
  }

  return config;
}

void DrumConfigMapper::convertGeneralConfigToJson(const DrumKit& drumKit, JsonDocument& config) {
  JsonObject generalNode = config[GENERAL_SECTION].to<JsonObject>();
  generalNode[GENERAL_GATETIME] = drumKit.getGateTime();
}
