// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config/config_mapper.h"
#include <LittleFS.h>
#include "yaml_parser.h"

///////////////////////////// Low-Level file access

static const char* CONFIG_FILE_PATH = "/config.yaml";

JsonDocument DrumConfigMapper::loadDrumKitConfig() {
    JsonDocument doc;
    JsonObject jsonObject = doc.to<JsonObject>();
  
    LittleFS.begin();
    fs::File localeFile = LittleFS.open(CONFIG_FILE_PATH, "r");
    if (!localeFile) {
      eventLog.log(Level::ERROR, String("Could not open Config: ") + CONFIG_FILE_PATH);
      return jsonObject;
    }
  
    YAMLParser::parseConfig(localeFile, jsonObject);
  
    localeFile.close();
    return jsonObject;
  }
  
  void DrumConfigMapper::writeDrumKitConfig(JsonDocument doc) {
    noInterrupts();
  
    fs::File localeFile = LittleFS.open(CONFIG_FILE_PATH, "w");
    localeFile.print("# yaml-language-server: $schema=./config.jsonc"); // Note: no newline, as YAMLduino already adds one
    serializeYml(doc, localeFile);
    localeFile.close();
  
    interrupts();
  }
  