// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"
#include "drum_kit.h"

#define PAD_NAME_PROP "name"
#define PAD_ROLE_PROP "role"
#define PAD_GROUP_PROP "group"
#define PAD_AUTOCALIBRATE_PROP "autocalibrate"
#define PAD_ENABLED_PROP "enabled"
#define PAD_PEDAL_PROP "pedal"
#define PAD_CONNECTOR_PROP "connector"

///////////////////////////// From JSON

void DrumConfigMapper::applyPadConfigs(DrumKit& drumKit, JsonArrayConst& padsNode) {
  if (!padsNode) {
    eventLog.log(Level::INFO, "Config: " PADS_SECTION " node missing");
    return;
  }

  for (pad_size_t padIndex = 0; padIndex < padsNode.size(); ++padIndex) {
    if (drumKit.getPadsCount() >= MAX_PAD_COUNT) {
      eventLog.log(Level::ERROR, String("Too many drum pads in config: ") + (int)padsNode.size() + " > " + MAX_PAD_COUNT);
      break;
    }

    DrumPad& pad = drumKit.addPad();
    applyPadConfig(pad, drumKit, padIndex, padsNode);
  }
}

void DrumConfigMapper::applyPadConfig(DrumPad& pad, DrumKit& drumKit, pad_size_t padIndex, JsonArrayConst& padsNode) {
  JsonObjectConst padNode = padsNode[padIndex];

  const char* name = padNode[PAD_NAME_PROP];
  pad.setName(name == nullptr ? String("Unknown") + padIndex : name);

  const char* role = padNode[PAD_ROLE_PROP];
  pad.setRole(role == nullptr ? String("Unknown") + padIndex : role);

  const char* groupName = padNode[PAD_GROUP_PROP] | "";
  pad.setGroup(groupName);

  const bool autoCalibrate = padNode[PAD_AUTOCALIBRATE_PROP] | false;
  pad.setAutoCalibrate(autoCalibrate);

  if (padNode[PAD_PEDAL_PROP].is<String>()) {
    String pedalRole = padNode[PAD_PEDAL_PROP];
    pad_size_t pedalIndex = findPedalIndexByRole(pedalRole, padsNode);
    if (pedalIndex == UNKNOWN_PAD) {
      eventLog.log(Level::ERROR, String("Pad[") + pad.getName() + "]: pedal with name '" + pedalRole + "' not found");
    } else {
      DrumPad* pedalPad = &drumKit.getPad(pedalIndex);
      pad.setPedalPad(pedalPad);
    }
  }

  bool failed = false;
  if (!padNode[PAD_CONNECTOR_PROP].is<String>()) {
    eventLog.log(Level::ERROR, String("Pad[") + pad.getName() + "]: property '" PAD_CONNECTOR_PROP "' missing");
    failed = true;
  } else {
    ConnectorId connectorId = padNode[PAD_CONNECTOR_PROP];
    DrumConnector* connector = drumKit.getConnectorById(connectorId);
    if (!connector) {
      eventLog.log(Level::ERROR, String("Pad[") + pad.getName() + "]: Connector[" + connectorId + "] is unknown");
      failed = true;
    }
    pad.setConnector(connector);
  }

  if (failed) {
    pad.setEnabled(false);
  } else {
    bool enabled = padNode[PAD_ENABLED_PROP];
    pad.setEnabled(enabled);
  }
}

pad_size_t DrumConfigMapper::findPedalIndexByRole(String pedalRole, JsonArrayConst& padsNode) {
  for (pad_size_t padIndex = 0; padIndex < padsNode.size(); ++padIndex) {
    auto padNode = padsNode[padIndex];
    if (String(padNode[PAD_ROLE_PROP]) == pedalRole) {
      return padIndex;
    }
  }
  return UNKNOWN_PAD;
}

///////////////////////////// To JSON

void DrumConfigMapper::convertPadConfigToJson(const DrumPad& pad, const DrumKit& drumKit, JsonObject& padConfigNode) {
  if (pad.getName() != "") {
    padConfigNode[PAD_NAME_PROP] = pad.getName();
  }

  if (pad.getRole() != "") {
    padConfigNode[PAD_ROLE_PROP] = pad.getRole();
  }

  if (pad.getGroup() != "") {
    padConfigNode[PAD_GROUP_PROP] = pad.getGroup();
  }

  if (pad.getAutoCalibrate()) {
    padConfigNode[PAD_AUTOCALIBRATE_PROP] = pad.getAutoCalibrate();
  }
  
  padConfigNode[PAD_ENABLED_PROP] = pad.isEnabled();

  DrumPad* pedalPad = pad.getPedalPad();
  if (pedalPad) {
    padConfigNode[PAD_PEDAL_PROP] = pedalPad->getName();
  }

  if (pad.getConnector()) {
    padConfigNode[PAD_CONNECTOR_PROP] = pad.getConnector()->getId();
  }
}
