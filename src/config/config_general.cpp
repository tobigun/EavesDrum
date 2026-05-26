// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"
#include "drum_kit.h"
#include "drum_io.h"
#include "midi_transport.h"

#if HAS_BLUETOOTH
#include "ble_client.h"
#include "wii_client.h"
#endif

#define GENERAL_BOARD "board"
#define GENERAL_GATETIME "gateTimeMs"
#define GENERAL_MIDI_OUTPUT_MODE "midiOutputMode"

#define GENERAL_BLE_PAIRING "blePairing"
#define GENERAL_BLE_PAIRING_NAME "name"
#define GENERAL_BLE_PAIRING_ADDR "address"

#define GENERAL_WII_PAIRING "wiiPairing"
#define GENERAL_WII_PAIRING_ADDR "address"
#define GENERAL_WII_PAIRING_LINK_KEY "key"

#define BOARD_V1_1 "v1.1"
#define BOARD_V1_2 "v1.2"

///////////////////////////// From JSON

void applyBoardConfig(DrumKit& drumKit, JsonObjectConst generalNode) {
  if (generalNode[GENERAL_BOARD].isUnbound()) {
    eventLog.log(Level::Info, "No board version specified (custom board)");
    return;
  }

  String boardStr = generalNode[GENERAL_BOARD].as<String>();
  if (boardStr != BOARD_V1_1 && boardStr != BOARD_V1_2) {
    eventLog.log(Level::Warn, String("Unknown board version: ") + boardStr);
    return;
  }

  BoardVersion boardVersion = BoardVersion::Custom;
  if (boardStr == BOARD_V1_1) {
    boardVersion = BoardVersion::V1_1;
  } else if (boardStr == BOARD_V1_2) {
    boardVersion = BoardVersion::V1_2;
  }
  drumKit.setBoardVersion(boardVersion);
  DrumIO::initBoard(boardVersion);

  eventLog.log(Level::Info, String("Board version: ") + boardStr);
}

void DrumConfigMapper::applyGeneralConfig(DrumKit& drumKit, JsonObjectConst generalNode) {
  if (!generalNode) {
    eventLog.log(Level::Info, "Config: " GENERAL_SECTION " node missing");
    return;
  }

  applyBoardConfig(drumKit, generalNode);

  if (generalNode[GENERAL_GATETIME].is<int>()) {
    drumKit.setGateTime(generalNode[GENERAL_GATETIME].as<int>());
  }

  if (generalNode[GENERAL_MIDI_OUTPUT_MODE].is<String>()) {
    String modeStr = generalNode[GENERAL_MIDI_OUTPUT_MODE].as<String>();
    MidiOutputMode mode = parseMidiOutputMode(modeStr);
    drumKit.setMidiOutputMode(mode);
  }

#if HAS_BLUETOOTH
  JsonObjectConst blePairingNode = generalNode[GENERAL_BLE_PAIRING];
  if (blePairingNode) {
    String address = blePairingNode[GENERAL_BLE_PAIRING_ADDR].as<String>();
    String name = blePairingNode[GENERAL_BLE_PAIRING_NAME].as<String>();
    bleClient.setPairingInfo(name, address);
  }
#endif

#ifdef ENABLE_MIDI_GUITAR_HERO_WII_TRANSPORT
  JsonObjectConst wiiPairingNode = generalNode[GENERAL_WII_PAIRING];
  if (wiiPairingNode) {
    String address = wiiPairingNode[GENERAL_WII_PAIRING_ADDR].as<String>();
    String linkKey = wiiPairingNode[GENERAL_WII_PAIRING_LINK_KEY].as<String>();
    wiiClient.setPairingInfo(address, linkKey);
  }
#endif
}

///////////////////////////// To JSON

void convertBoardConfigToJson(const DrumKit& drumKit, JsonObject generalNode) {
  if (drumKit.getBoardVersion() == BoardVersion::V1_1) {
    generalNode[GENERAL_BOARD] = BOARD_V1_1;
  } else if (drumKit.getBoardVersion() == BoardVersion::V1_2) {
    generalNode[GENERAL_BOARD] = BOARD_V1_2;
  }
}

void DrumConfigMapper::convertGeneralConfigToJson(const DrumKit& drumKit, JsonDocument& config) {
  JsonObject generalNode = config[GENERAL_SECTION].to<JsonObject>();

  convertBoardConfigToJson(drumKit, generalNode);

  generalNode[GENERAL_GATETIME] = drumKit.getGateTime();

  generalNode[GENERAL_MIDI_OUTPUT_MODE] = midiOutputModeToString(drumKit.getMidiOutputMode());

#if HAS_BLUETOOTH
  if (!bleClient.getPairingInfo().address.isEmpty()) {
    JsonObject bleNode = generalNode[GENERAL_BLE_PAIRING].to<JsonObject>();
    bleNode[GENERAL_BLE_PAIRING_ADDR] = bleClient.getPairingInfo().address;
    bleNode[GENERAL_BLE_PAIRING_NAME] = bleClient.getPairingInfo().name;
  }
#endif

#ifdef ENABLE_MIDI_GUITAR_HERO_WII_TRANSPORT
  if (!wiiClient.getPairingInfo().address.isEmpty()) {
    JsonObject wiiNode = generalNode[GENERAL_WII_PAIRING].to<JsonObject>();
    wiiNode[GENERAL_WII_PAIRING_ADDR] = wiiClient.getPairingInfo().address;
    wiiNode[GENERAL_WII_PAIRING_LINK_KEY] = wiiClient.getPairingInfo().linkKey;
  }
#endif
}
