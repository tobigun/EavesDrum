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
  addMultiplexersToKit(drumKit, muxNodes);

  JsonObjectConst connectorsNodes = configNode[CONNECTORS_SECTION];
  addConnectorsToDrumKit(drumKit, connectorsNodes);
  
  JsonArrayConst padsNode = configNode[PADS_SECTION];
  addPadsToDrumKit(drumKit, padsNode);

  JsonObjectConst mappingsNode = configNode[MAPPINGS_SECTION];
  if (!mappingsNode) {
    eventLog.log(Level::ERROR, "Config: mappings node missing");
  } else {
    applyDrumKitMappings(drumKit, mappingsNode, false);
  }

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

/**
 * Applies mappings from JSON to the drum kit. Existing mapping entries are either merged with or replaced by the new ones.
 * Note that this does not modify or remove mappings for roles that are not contained in this request, but it can add new roles.
 * 
 * @param replace if true, the mappings for the roles contained in this request will be replaced with defaults before applying the new values.
 *  If false, for each role contained in this request the new mappings will be applied on top of the existing ones, effectively merging old and new values.
 */
void DrumConfigMapper::applyDrumKitMappings(DrumKit& drumKit, JsonObjectConst mappingsNode, bool replace) {
  for (JsonPairConst mappingsKeyValuePair : mappingsNode) {
    String role = mappingsKeyValuePair.key().c_str();
    DrumMappings* mappings = drumKit.getOrCreateMappings(role);
    if (!mappings) { // mapping not found and could not be created
      continue;
    }

    if (replace) {
      *mappings = DrumMappings(role); // replace existing mappings with defaults
    }

    JsonObjectConst mappingValuesNode = mappingsKeyValuePair.value();
    DrumConfigMapper::applyMappings(*mappings, mappingValuesNode);

    DrumPad* pad = drumKit.getPadByRole(role);
    if (!pad) {
      // Note: not all roles have to be used, so this is just a warning
      eventLog.log(Level::INFO, String("Config: unused mappings with role: ") + role);
    } else {
      pad->setMappings(mappings);
    }
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
  for (pad_size_t padIndex = 0; padIndex < drumKit.getPadsCount(); ++padIndex) {
    const DrumPad& pad = *drumKit.getPad(padIndex);
    JsonObject padConfigNode = padsNode[padIndex].to<JsonObject>();
    convertPadConfigToJson(pad, drumKit, padConfigNode);
  }

  JsonObject mappingsNode = config[MAPPINGS_SECTION].to<JsonObject>();
  for (mappings_size_t mappingsIndex = 0; mappingsIndex < drumKit.getMappingsCount(); ++mappingsIndex) {
    const DrumMappings& mappings = *drumKit.getMappings(mappingsIndex);

    JsonObject mappingValuesNode = mappingsNode[mappings.role].to<JsonObject>();
    convertMappingsToJson(mappings, mappingValuesNode);
  }

  return config;
}

void DrumConfigMapper::convertGeneralConfigToJson(const DrumKit& drumKit, JsonDocument& config) {
  JsonObject generalNode = config[GENERAL_SECTION].to<JsonObject>();
  generalNode[GENERAL_GATETIME] = drumKit.getGateTime();
}
