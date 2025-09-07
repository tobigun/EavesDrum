// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"
#include "drum_kit.h"

#define CONNECTOR_PINS_PROP "pins"
#define CONNECTOR_PINS_MUX_PROP "mux"
#define CONNECTOR_PINS_CHANNEL_PROP "channel"

///////////////////////////// From JSON

void DrumConfigMapper::addConnectorsToDrumKit(DrumKit& drumKit, JsonObjectConst connectorsNode) {
  if (!connectorsNode) {
    eventLog.log(Level::INFO, "Config: " CONNECTORS_SECTION " node missing");
    return;
  }

  for (JsonPairConst connectorKeyValuePair : connectorsNode) {
    if (drumKit.getConnectorsCount() >= MAX_CONNECTOR_COUNT) {
      eventLog.log(Level::ERROR, String("Too many connectors in config: ") + (int)connectorsNode.size() + " > " + MAX_CONNECTOR_COUNT);
      break;
    }

    ConnectorId id = ConnectorId(connectorKeyValuePair.key().c_str());
    if (drumKit.getConnectorById(id)) {
      eventLog.log(Level::ERROR, String("Connector with ID '") + id + "' already present -> skip");
      continue;
    }

    DrumConnector connector;
    connector.setId(id);
    JsonObjectConst connectorNode = connectorKeyValuePair.value();
    applyPinsConfig(connector, drumKit, connectorNode[CONNECTOR_PINS_PROP]);
    drumKit.addConnector(connector);
  }
}

bool DrumConfigMapper::applyPinsConfig(DrumConnector& connector, DrumKit& drumKit, JsonArrayConst pinsNode) {
  const pin_size_t MAX_PINS = 3;
  DrumPin pins[MAX_PINS];

  pin_size_t pinCount = pinsNode.size();

  if (pinCount == 0) {
    eventLog.log(Level::ERROR, String("Connector[") + connector.getId() + "] must have at least one pin definition");
    return false;
  }

  if (pinCount > MAX_PINS) {
    pinCount = MAX_PINS;
    eventLog.log(Level::WARN, String("Connector[") + connector.getId() + "] has more than " + MAX_PINS + " pins");
  }

  for (pin_size_t i = 0; i < pinCount; ++i) {
    pins[i] = getPinsConfig(connector, drumKit, pinsNode[i]);
    if (!pins[i].isValid()) {
      return false;
    }
  }

  connector.setPins(pins, pinCount);

  return true;
}

DrumPin DrumConfigMapper::getPinsConfig(DrumConnector& connector, DrumKit& drumKit, JsonVariantConst pinsNode) {
  if (pinsNode.isNull()) {
    return DrumPin();
  }

  // direct pin
  if (pinsNode.is<pin_size_t>()) {
    return DrumPin(pinsNode.as<pin_size_t>());
  }

  // mux pin

  JsonVariantConst muxNode = pinsNode[CONNECTOR_PINS_MUX_PROP];
  JsonVariantConst channelNode = pinsNode[CONNECTOR_PINS_CHANNEL_PROP];
  if (muxNode.isNull() || channelNode.isNull()) {
    eventLog.log(Level::ERROR, String("Connector[") + connector.getId() + "]: " CONNECTOR_PINS_PROP " entry is neither a pin nor mux channel");
    return DrumPin();
  }

  mux_size_t muxIndex = muxNode.as<mux_size_t>();
  DrumMux* mux = drumKit.getMux(muxIndex);
  if (!mux) {
    eventLog.log(Level::ERROR, String("Connector[") + connector.getId() + "]: Mux with index '" + muxIndex + "' does not exist");
    return DrumPin();
  }

  channel_size_t channelIndex = channelNode.as<channel_size_t>();
  if (channelIndex >= mux->getChannelCount()) {
    eventLog.log(Level::ERROR, String("Connector[") + connector.getId() + "]: Mux does not have channel with index " + channelIndex);
    return DrumPin();
  }

  return DrumPin(mux, muxIndex, channelIndex);
}

///////////////////////////// To JSON

void DrumConfigMapper::convertConnectorConfigsToJson(const DrumKit& drumKit, JsonDocument& config) {
  if (drumKit.getConnectorsCount() == 0) {
    return;
  }

  JsonObject connectorsNode = config[CONNECTORS_SECTION].to<JsonObject>();
  for (connector_size_t connIndex = 0; connIndex < drumKit.getConnectorsCount(); ++connIndex) {
    const DrumConnector* connector = drumKit.getConnector(connIndex);
    if (connector) {
      JsonObject connectorNode = connectorsNode[connector->getId()].to<JsonObject>();
      convertConnectorConfigToJson(*connector, drumKit, connectorNode);
    }
  }
}

void DrumConfigMapper::convertConnectorConfigToJson(const DrumConnector& connector, const DrumKit& drumKit, JsonObject& connectorNode) {
  JsonArray pinsNode = connectorNode[CONNECTOR_PINS_PROP].to<JsonArray>();
  convertPinsToJson(connector, pinsNode);
}

void DrumConfigMapper::convertPinsToJson(const DrumConnector& connector, JsonArray pinsNode) {
  pin_size_t count = connector.getPinCount();
  for (pin_size_t i = 0; i < count; ++i) {
    const DrumPin& pin = connector.getPin(i);
    if (!pin.mux) {
      pinsNode.add(pin.index);
    } else {
      JsonObject muxNode = pinsNode.add<JsonObject>();
      muxNode[CONNECTOR_PINS_MUX_PROP] = pin.muxIndex;
      muxNode[CONNECTOR_PINS_CHANNEL_PROP] = pin.index;
    }
  }
}
