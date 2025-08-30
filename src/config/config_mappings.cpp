// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"

///////////////////////////// From JSON

#define MAPPING_FROM_JSON(name, type) \
  if (mappingsNode[#name].is<type>()) \
    mappings.name = mappingsNode[#name].as<type>();

#define MAPPING_NOTE_FROM_JSON(name) \
  MAPPING_FROM_JSON(name, int)       \
  mappings.name = mappings.name > 127 ? 0 : mappings.name;

void DrumConfigMapper::applyPadMappings(DrumPad& pad, JsonObjectConst& mappingsNode) {
  DrumMappings& mappings = pad.getMappings();

  MAPPING_FROM_JSON(name, String) 

  MAPPING_NOTE_FROM_JSON(noteMain)
  MAPPING_NOTE_FROM_JSON(noteRim)
  MAPPING_NOTE_FROM_JSON(noteCup)

  MAPPING_NOTE_FROM_JSON(noteCross)

  MAPPING_FROM_JSON(closedNotesEnabled, bool)
  MAPPING_NOTE_FROM_JSON(noteCloseMain)
  MAPPING_NOTE_FROM_JSON(noteCloseRim)
  MAPPING_NOTE_FROM_JSON(noteCloseCup)

  MAPPING_FROM_JSON(pedalChickEnabled, bool)
}

///////////////////////////// To JSON

static const DrumMappingId mappingItemsDrum[] = {
    NOTE_MAIN,
    NOTE_RIM,
    NOTE_CROSS,
};
const int MAPPING_ITEMS_DRUM_SIZES[3] = {1, 4, 4};

static const DrumMappingId mappingItemsCymbal[] = {
    NOTE_MAIN,
    NOTE_RIM,
    NOTE_CUP,
};
const int MAPPING_ITEMS_CYMBAL_SIZES[3] = {1, 2, 3};

static const DrumMappingId mappingItemsHiHat[] = {
    ENABLE_CLOSED_NOTES,
    NOTE_MAIN,
    NOTE_CLOSE_MAIN,
    NOTE_RIM,
    NOTE_CLOSE_RIM,
    NOTE_CUP,
    NOTE_CLOSE_CUP,
};
const int MAPPING_ITEMS_HIHAT_SIZE = sizeof(mappingItemsHiHat) / sizeof(DrumMappingId);
const int MAPPING_ITEMS_HIHAT_SIZES[3] = {
    MAPPING_ITEMS_HIHAT_SIZE - 4,
    MAPPING_ITEMS_HIHAT_SIZE - 2,
    MAPPING_ITEMS_HIHAT_SIZE,
};

static const DrumMappingId mappingItemsPedalControl[] = {
    ENABLE_PEDAL_CHICK,
    NOTE_MAIN,
};
const int MAPPING_ITEMS_PEDAL_CONTROL_SIZE = sizeof(mappingItemsPedalControl) / sizeof(DrumMappingId);

#define MAPPING_TO_JSON(label, ymlName)        \
  if (mappingId == label) {                    \
    mappingsNode[#ymlName] = mappings.ymlName; \
    return;                                    \
  }

static void convertPadMappingToJson(DrumMappingId mappingId, JsonObject mappingsNode, const DrumMappings& mappings) {
  MAPPING_TO_JSON(NOTE_MAIN, noteMain)
  MAPPING_TO_JSON(NOTE_RIM, noteRim)
  MAPPING_TO_JSON(NOTE_CUP, noteCup)

  MAPPING_TO_JSON(NOTE_CROSS, noteCross)

  MAPPING_TO_JSON(ENABLE_CLOSED_NOTES, closedNotesEnabled)
  MAPPING_TO_JSON(NOTE_CLOSE_MAIN, noteCloseMain)
  MAPPING_TO_JSON(NOTE_CLOSE_RIM, noteCloseRim)
  MAPPING_TO_JSON(NOTE_CLOSE_CUP, noteCloseCup)

  MAPPING_TO_JSON(ENABLE_PEDAL_CHICK, pedalChickEnabled)
}

static int getSupportedMappings(const DrumPad& pad, const DrumMappingId*& mappings) {
  zone_size_t zoneCount = pad.getSettings().getZoneCount();
  switch (pad.getPadType()) {
  case PadType::Drum:
    mappings = mappingItemsDrum;
    return MAPPING_ITEMS_DRUM_SIZES[zoneCount - 1];
  case PadType::Cymbal:
    if (pad.getPedalPad()) { // HiHat
      mappings = mappingItemsHiHat;
      return MAPPING_ITEMS_HIHAT_SIZES[zoneCount - 1];
    } else {
      mappings = mappingItemsCymbal;
      return MAPPING_ITEMS_CYMBAL_SIZES[zoneCount - 1];
    }
  case PadType::Pedal:
    mappings = mappingItemsPedalControl;
    return MAPPING_ITEMS_PEDAL_CONTROL_SIZE;
  default:
    return 0;
  }
}

void DrumConfigMapper::convertPadMappingsToJson(const DrumPad& pad, const DrumKit& drumKit, JsonObject& mappingsNode) {
  const DrumMappings& padMappings = pad.getMappings();

  if (padMappings.name) {
    mappingsNode["name"] = padMappings.name;
  }

  const DrumMappingId* mappingIds;
  int mappingsCount = getSupportedMappings(pad, mappingIds);
  for (int i = 0; i < mappingsCount; ++i) {
    convertPadMappingToJson(mappingIds[i], mappingsNode, padMappings);
  }
}
