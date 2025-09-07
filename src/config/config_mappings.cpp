// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"

///////////////////////////// From JSON

// setting a field to null resets the value to null/undefined, whereas leaving a field undefined keeps the current value
#define IS_EXPLICITLY_NULL(node) (!node.isUnbound() && node.isNull())

#define STRING_MAPPING_FROM_JSON(name) \
  if (mappingsNode[#name].is<String>()) { \
    mappings.name = mappingsNode[#name].as<String>(); \
  }

#define BOOL_MAPPING_FROM_JSON(name) \
  if (mappingsNode[#name].is<bool>()) { \
    mappings.name = mappingsNode[#name].as<bool>(); \
  } else if (IS_EXPLICITLY_NULL(mappingsNode[#name])) { \
    mappings.name = BOOL_UNDEFINED; \
  }

#define NOTE_MAPPING_FROM_JSON(name) \
  if (mappingsNode[#name].is<int>()) { \
    int _value = mappingsNode[#name].as<int>(); \
    mappings.name = _value > 127 ? 0 : _value; \
  } else if (IS_EXPLICITLY_NULL(mappingsNode[#name])) { \
    mappings.name = MIDI_NOTE_UNASSIGNED; \
  }

void DrumConfigMapper::applyMappings(DrumMappings& mappings, JsonObjectConst& mappingsNode) {
  STRING_MAPPING_FROM_JSON(name) 

  NOTE_MAPPING_FROM_JSON(noteMain)
  NOTE_MAPPING_FROM_JSON(noteRim)
  NOTE_MAPPING_FROM_JSON(noteCup)

  NOTE_MAPPING_FROM_JSON(noteCross)

  BOOL_MAPPING_FROM_JSON(closedNotesEnabled)
  NOTE_MAPPING_FROM_JSON(noteCloseMain)
  NOTE_MAPPING_FROM_JSON(noteCloseRim)
  NOTE_MAPPING_FROM_JSON(noteCloseCup)

  BOOL_MAPPING_FROM_JSON(pedalChickEnabled)
}

///////////////////////////// To JSON

#define STRING_MAPPING_TO_JSON(ymlName) \
  if (mappings.ymlName) { \
    mappingsNode[#ymlName] = mappings.ymlName; \
  }

#define NOTE_MAPPING_TO_JSON(ymlName) \
  if (mappings.ymlName != MIDI_NOTE_UNASSIGNED) { \
    mappingsNode[#ymlName] = mappings.ymlName; \
  }

#define BOOL_MAPPING_TO_JSON(ymlName) \
  if (mappings.ymlName != BOOL_UNDEFINED) { \
    mappingsNode[#ymlName] = (bool) mappings.ymlName; \
  }

void DrumConfigMapper::convertMappingsToJson(const DrumMappings& mappings, JsonObject& mappingsNode) {
  STRING_MAPPING_TO_JSON(name)
  
  NOTE_MAPPING_TO_JSON(noteMain)
  NOTE_MAPPING_TO_JSON(noteRim)
  NOTE_MAPPING_TO_JSON(noteCup)

  NOTE_MAPPING_TO_JSON(noteCross)

  BOOL_MAPPING_TO_JSON(closedNotesEnabled)
  NOTE_MAPPING_TO_JSON(noteCloseMain)
  NOTE_MAPPING_TO_JSON(noteCloseRim)
  NOTE_MAPPING_TO_JSON(noteCloseCup)

  BOOL_MAPPING_TO_JSON(pedalChickEnabled)
}
