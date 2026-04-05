// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config/config_mapper.h"
#include "config_fs.h"
#include "yaml_parser.h"

#include <LittleFS.h>

#define CONFIG_SCHEMA_VERSION "1.1"
#define CONFIG_SCHEMA_URL "https://raw.githubusercontent.com/tobigun/EavesDrum/refs/tags/" \
  "config-schema-v" CONFIG_SCHEMA_VERSION "/config/config.jsonc"

///////////////////////////// Low-Level file access

static const char* CONFIG_FILE_PATH = "/config.yaml";

void printConfig(JsonDocument& doc) {
  String output;
  serializeYml(doc, output);
  SerialDebug.println("Config file read content:\n" + output);
}

JsonDocument loadConfigFile(fs::File& file) {
  JsonDocument doc;
  JsonObject jsonObject = doc.to<JsonObject>();

  YAMLParser::parseConfig(file, jsonObject);
  //printConfig(doc);

  file.close();
  return jsonObject;
}

JsonDocument loadInitialConfigFile() {
  eventLog.log(Level::Info, String("Fallback to initial config file: ") + CONFIG_FILE_PATH);

  LittleFS.begin();
  fs::File defaultFile = LittleFS.open(CONFIG_FILE_PATH, "r");
  if (!defaultFile) {
    eventLog.log(Level::Error, String("Could not open default config: ") + CONFIG_FILE_PATH);
    return JsonDocument().to<JsonObject>();
  }

  return loadConfigFile(defaultFile);
}

JsonDocument DrumConfigMapper::loadDrumKitConfig() {
  ConfigFS.begin();
  fs::File configFile = ConfigFS.open(CONFIG_FILE_PATH, "r");
  if (!configFile) {
    // fallback to default from read-only FS
    return loadInitialConfigFile();
  }

  return loadConfigFile(configFile);
}

bool DrumConfigMapper::writeDrumKitConfig(JsonDocument doc) {
  bool writeSuccess = false;
  noInterrupts();

  fs::File configFile = ConfigFS.open(CONFIG_FILE_PATH, "w");
  if (!configFile) {
    eventLog.log(Level::Error, String("Could not open config for writing: ") + CONFIG_FILE_PATH);
  } else {
    //printConfig(doc);

    configFile.print("# yaml-language-server: $schema=" CONFIG_SCHEMA_URL); // Note: no newline, as YAMLduino already adds one
    serializeYml(doc, configFile);
    configFile.close();
    writeSuccess = true;
  }

  interrupts();
  return writeSuccess;
}
