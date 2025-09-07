// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"
#include "drum_kit.h"

#define MUX_TYPE_PROP "type"
#define MUX_PINS_PROP "pins"
#define MUX_PINS_SELECT_PROP "select"
#define MUX_PINS_ANALOGIN_PROP "analogIn"
#define MUX_PINS_ENABLE_PROP "enable"

///////////////////////////// From JSON

static MuxType mapStringToMuxType(String value) {
  using enum MuxType;
  MAP_STRING_TO_ENUM(HC4051);
  MAP_STRING_TO_ENUM(HC4067);
  return Unknown;
}

void DrumConfigMapper::addMultiplexersToKit(DrumKit& drumKit, JsonArrayConst& muxNodes) {
  for (JsonObjectConst muxNode : muxNodes) {
    if (drumKit.getMuxCount() >= MAX_MUX_COUNT) {
      eventLog.log(Level::ERROR, String("Max. mux count reached: ") + MAX_MUX_COUNT);
      break;
    }

    DrumMux mux;
    if (applyMuxConfig(mux, muxNode)) {
      drumKit.addMux(mux);
      eventLog.log(Level::INFO, mux);
    }
  }
}

bool DrumConfigMapper::applyMuxConfig(DrumMux& mux, JsonObjectConst& muxNode) {
  MuxType type = mapStringToMuxType(muxNode[MUX_TYPE_PROP]);
  if (type == MuxType::Unknown) {
    eventLog.log(Level::ERROR, String("Invalid mux type: ") + muxNode[MUX_TYPE_PROP].as<String>());
    return false;
  }

  JsonObjectConst pinsNode = muxNode[MUX_PINS_PROP];
  if (!pinsNode) {
    eventLog.log(Level::ERROR, MUX_SECTION "." MUX_PINS_PROP " node missing");
    return false;
  }

  JsonArrayConst selectPinsNode = pinsNode[MUX_PINS_SELECT_PROP];
  if (!selectPinsNode) {
    eventLog.log(Level::ERROR, MUX_SECTION "." MUX_PINS_PROP "." MUX_PINS_SELECT_PROP " node missing");
    return false;
  }

  pin_size_t numSelectPins = selectPinsNode.size();
  if (type == MuxType::HC4051 && numSelectPins != 3) {
    eventLog.log(Level::ERROR, String("HC4051 requires 3 select pins, got ") + numSelectPins);
    return false;
  } else if (type == MuxType::HC4067 && numSelectPins != 4) {
    eventLog.log(Level::ERROR, String("HC4067 requires 4 select pins, got ") + numSelectPins);
    return false;
  }

  pin_size_t selectPins[4] = {PIN_UNUSED, PIN_UNUSED, PIN_UNUSED, PIN_UNUSED}; // HC4051: 3 pins, HC4067: 4 pins
  for (pin_size_t i = 0; i < selectPinsNode.size(); ++i) {
    selectPins[i] = selectPinsNode[i];
  }

  if (pinsNode[MUX_PINS_ANALOGIN_PROP].isNull()) {
    eventLog.log(Level::ERROR, MUX_SECTION "." MUX_PINS_PROP "." MUX_PINS_ANALOGIN_PROP " node missing");
    return false;
  }

  pin_size_t analogInPin = pinsNode[MUX_PINS_ANALOGIN_PROP];
  pin_size_t enablePin = pinsNode[MUX_PINS_ENABLE_PROP].isNull() ? PIN_UNUSED : pinsNode[MUX_PINS_ENABLE_PROP];

  if (type == MuxType::HC4051) {
    mux.initHC4051(selectPins[0], selectPins[1], selectPins[2],
        analogInPin, enablePin);
  } else {
    mux.initHC4067(selectPins[0], selectPins[1], selectPins[2], selectPins[3],
        analogInPin, enablePin);
  }

  return true;
}

///////////////////////////// To JSON

static String mapMuxTypeToString(MuxType value) {
  using enum MuxType;
  MAP_ENUM_TO_STRING(HC4051);
  MAP_ENUM_TO_STRING(HC4067);
  return ENUM_TO_STRING(Unknown);
}

void DrumConfigMapper::convertMuxConfigsToJson(const DrumKit& drumKit, JsonDocument& config) {
  if (drumKit.getMuxCount() == 0) {
    return;
  }

  JsonArray muxersNode = config[MUX_SECTION].to<JsonArray>();
  for (mux_size_t muxIndex = 0; muxIndex < drumKit.getMuxCount(); ++muxIndex) {
    JsonObject muxNode = muxersNode.add<JsonObject>();
    convertMuxConfigToJson(*drumKit.getMux(muxIndex), drumKit, muxNode);
  }
}

void DrumConfigMapper::convertMuxConfigToJson(const DrumMux& mux, const DrumKit& drumKit, JsonObject& muxNode) {
  muxNode[MUX_TYPE_PROP] = mapMuxTypeToString(mux.getMuxType());
  JsonObject pinsNode = muxNode[MUX_PINS_PROP].to<JsonObject>();
  pinsNode[MUX_PINS_ANALOGIN_PROP] = mux.getAnalogInPin();
  if (mux.getEnablePin() != PIN_UNUSED) {
    pinsNode[MUX_PINS_ENABLE_PROP] = mux.getEnablePin();
  }
  for (pin_size_t i = 0; i < mux.getSelectPinsCount(); ++i) {
    pinsNode[MUX_PINS_SELECT_PROP].add(mux.getSelectPin(i));
  }
}
