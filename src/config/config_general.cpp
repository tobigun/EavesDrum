// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"
#include "drum_kit.h"
#include "midi_transport.h"

#if HAS_BLUETOOTH
#include "ble_client.h"
#endif

#define GENERAL_BOARD "board"
#define GENERAL_GATETIME "gateTimeMs"
#define GENERAL_MIDI_OUTPUT_MODE "midiOutputMode"

#define GENERAL_BLE_PAIRING "blePairing"
#define GENERAL_BLE_PAIRING_NAME "name"
#define GENERAL_BLE_PAIRING_ADDR "address"

///////////////////////////// From JSON

void DrumConfigMapper::applyGeneralConfig(DrumKit& drumKit, JsonObjectConst generalNode) {
  if (!generalNode) {
    eventLog.log(Level::Info, "Config: " GENERAL_SECTION " node missing");
    return;
  }

  if (generalNode[GENERAL_BOARD].is<String>()) {
    String boardStr = generalNode[GENERAL_BOARD].as<String>();
    if (boardStr == "1.1") {
      drumKit.setBoardVersion(BoardVersion::V1_1);
    } else if (boardStr == "1.2") {
      drumKit.setBoardVersion(BoardVersion::V1_2);
    } else {
      eventLog.log(Level::Warn, String("Unknown board version: ") + boardStr);
    }
  }

  if (generalNode[GENERAL_GATETIME].is<int>()) {
    drumKit.setGateTime(generalNode[GENERAL_GATETIME].as<int>());
  }

  if (generalNode[GENERAL_MIDI_OUTPUT_MODE].is<String>()) {
    String modeStr = generalNode[GENERAL_MIDI_OUTPUT_MODE].as<String>();
    MidiOutputMode mode = parseMidiOutputMode(modeStr);
    drumKit.setMidiOutputMode(mode);
  }

#if HAS_BLUETOOTH
  JsonObjectConst paringNode = generalNode[GENERAL_BLE_PAIRING];
  if (paringNode) {
    String address = paringNode[GENERAL_BLE_PAIRING_ADDR].as<String>();
    String name = paringNode[GENERAL_BLE_PAIRING_NAME].as<String>();
    bleClient.setPairingInfo(name, address);
  }
#endif
}

///////////////////////////// To JSON

void DrumConfigMapper::convertGeneralConfigToJson(const DrumKit& drumKit, JsonDocument& config) {
  JsonObject generalNode = config[GENERAL_SECTION].to<JsonObject>();

  if (drumKit.getBoardVersion() == BoardVersion::V1_1) {
    generalNode[GENERAL_BOARD] = "1.1";
  } else if (drumKit.getBoardVersion() == BoardVersion::V1_2) {
    generalNode[GENERAL_BOARD] = "1.2";
  }

  generalNode[GENERAL_GATETIME] = drumKit.getGateTime();

  generalNode[GENERAL_MIDI_OUTPUT_MODE] = midiOutputModeToString(drumKit.getMidiOutputMode());

#if HAS_BLUETOOTH
  if (!bleClient.getPairingInfo().address.isEmpty()) {
    JsonObject bleNode = generalNode[GENERAL_BLE_PAIRING].to<JsonObject>();
    bleNode[GENERAL_BLE_PAIRING_ADDR] = bleClient.getPairingInfo().address;
    bleNode[GENERAL_BLE_PAIRING_NAME] = bleClient.getPairingInfo().name;
  }
#endif
}
