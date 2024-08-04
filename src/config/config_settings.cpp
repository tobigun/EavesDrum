// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config_mapper.h"

#define SETTINGS_ZONES_TYPE_PROP "zonesType"
#define SETTINGS_CHOKE_TYPE_PROP "chokeType"
#define SETTINGS_CURVETYPE_PROP "curveType"
#define SETTINGS_PAD_TYPE_PROP "padType"

///////////////////////////// From JSON

static PadType mapStringToPadType(String value) {
  using enum PadType;
  MAP_STRING_TO_ENUM(Drum);
  MAP_STRING_TO_ENUM(Cymbal);
  MAP_STRING_TO_ENUM(Pedal);
  eventLog.log(Level::WARN, String("Unknown pad type '") + value + "' -> using fallback ");
  return PAD_TYPE_DEFAULT;
}

static ZonesType mapStringToZonesType(String value) {
  using enum ZonesType;
  MAP_STRING_TO_ENUM(Zones1_Controller);
  MAP_STRING_TO_ENUM(Zones1_Piezo);
  MAP_STRING_TO_ENUM(Zones2_Piezos);
  MAP_STRING_TO_ENUM(Zones2_PiezoAndSwitch);
  MAP_STRING_TO_ENUM(Zones3_Piezos);
  MAP_STRING_TO_ENUM(Zones3_PiezoAndSwitches_2TRS);
  MAP_STRING_TO_ENUM(Zones3_PiezoAndSwitches_1TRS);
  eventLog.log(Level::WARN, String("Unknown zones type '") + value + "' -> using fallback");
  return ZONES_TYPE_DEFAULT;
}

static ChokeType mapStringToChokeType(String value) {
  using enum ChokeType;
  MAP_STRING_TO_ENUM(None);
  MAP_STRING_TO_ENUM(Switch_Edge);
  MAP_STRING_TO_ENUM(Switch_Cup);
  // MAP_STRING_TO_ENUM(TouchSensor);
  eventLog.log(Level::WARN, String("Unknown choke type '") + value + "' -> using fallback");
  return None;
}

static CurveType mapStringToCurveType(String value) {
  using enum CurveType;
  MAP_STRING_TO_ENUM(Linear);
  MAP_STRING_TO_ENUM(Exponential1);
  MAP_STRING_TO_ENUM(Exponential2);
  MAP_STRING_TO_ENUM(Logarithmic1);
  MAP_STRING_TO_ENUM(Logarithmic2);
  eventLog.log(Level::WARN, String("Unknown curve type '") + value + "' -> using fallback");
  return CURVE_TYPE_DEFAULT;
}

template <class T>
static inline void SETTING_FROM_JSON(T& setting, JsonVariantConst settingsNode) {
  if (settingsNode.is<T>()) {
    setting = settingsNode.as<T>();
  }
}

// Note: this will also be called on reconfiguration with a partial config
void DrumConfigMapper::applyPadSettings(DrumPad& pad, JsonObjectConst settingsNode) {
  DrumSettings& settings = pad.getSettings();

  if (settingsNode[SETTINGS_PAD_TYPE_PROP]) {
    settings.padType = mapStringToPadType(settingsNode[SETTINGS_PAD_TYPE_PROP]);
  }
  if (settingsNode[SETTINGS_ZONES_TYPE_PROP]) {
    settings.zonesType = mapStringToZonesType(settingsNode[SETTINGS_ZONES_TYPE_PROP]);
  }
  if (settingsNode[SETTINGS_CHOKE_TYPE_PROP]) {
    settings.chokeType = mapStringToChokeType(settingsNode[SETTINGS_CHOKE_TYPE_PROP]);
  }
  if (settingsNode[SETTINGS_CURVETYPE_PROP]) {
    settings.curveType = mapStringToCurveType(settingsNode[SETTINGS_CURVETYPE_PROP]);
  }

  JsonArrayConst zoneThresholdsNode = settingsNode["zoneThresholds"];
  if (!zoneThresholdsNode.isNull()) {
    for (int zone = 0; zone < 3; ++zone) {
      JsonObjectConst thresholds = zoneThresholdsNode[zone];
      if (!thresholds.isNull()) {
        SETTING_FROM_JSON(settings.zoneThresholdsMin[zone], thresholds["min"]);
        SETTING_FROM_JSON(settings.zoneThresholdsMax[zone], thresholds["max"]);
      }
    }
  }

  SETTING_FROM_JSON(settings.scanTimeUs, settingsNode["scanTimeUs"]);
  SETTING_FROM_JSON(settings.maskTimeMs, settingsNode["maskTimeMs"]);
  SETTING_FROM_JSON(settings.almostClosedThreshold, settingsNode["almostClosedThreshold"]);
  SETTING_FROM_JSON(settings.closedThreshold, settingsNode["closedThreshold"]);
  SETTING_FROM_JSON(settings.moveDetectTolerance, settingsNode["moveDetectTolerance"]);
  SETTING_FROM_JSON(settings.chickDetectTimeoutMs, settingsNode["chickDetectTimeoutMs"]);
  SETTING_FROM_JSON(settings.crossNoteEnabled, settingsNode["crossNoteEnabled"]);
  SETTING_FROM_JSON(settings.headRimBias, settingsNode["headRimBias"]);
}

///////////////////////////// To JSON

#define SETTING_ITEMS_GENERIC \
  PAD_TYPE, ENABLED, ZONES_TYPE

enum DrumSettingId {
  PAD_TYPE,
  ZONES_TYPE,
  CHOKE_TYPE,
  ENABLED,

  CURVETYPE,
  SCAN_TIME_MS,
  MASK_TIME_MS,

  ZONE_THRESHOLDS,
  THRESHOLD_MAX,

  ALMOST_CLOSED_THRESHOLD,
  CLOSED_THRESHOLD,
  MOVE_DETECT_TOLERANCE,
  CHICK_DETECT_TIMEOUT,
  HEAD_RIM_BIAS,
  ENABLE_CROSS_NOTE,
};

static const DrumSettingId settingItemsFallback[] = {
    SETTING_ITEMS_GENERIC};
const int SETTING_ITEMS_FALLBACK_SIZE = sizeof(settingItemsFallback) / sizeof(DrumSettingId);

static const DrumSettingId settingItemsPiezoOnly[] = {
    SETTING_ITEMS_GENERIC,
    CURVETYPE,
    SCAN_TIME_MS,
    MASK_TIME_MS,
    ZONE_THRESHOLDS};
const int SETTING_ITEMS_PIEZO_ONLY_SIZE = sizeof(settingItemsPiezoOnly) / sizeof(DrumSettingId);

static const DrumSettingId settingItemsPiezoAndSwitches[] = {
    SETTING_ITEMS_GENERIC,
    CHOKE_TYPE,
    CURVETYPE,
    SCAN_TIME_MS,
    MASK_TIME_MS,
    ZONE_THRESHOLDS};
const int SETTING_ITEMS_PIEZO_AND_SWITCHES_SIZE = sizeof(settingItemsPiezoAndSwitches) / sizeof(DrumSettingId);

static const DrumSettingId settingItemsPedalControl[] = {
    SETTING_ITEMS_GENERIC,
    ZONE_THRESHOLDS,
    ALMOST_CLOSED_THRESHOLD,
    CLOSED_THRESHOLD,
    MOVE_DETECT_TOLERANCE,
    CHICK_DETECT_TIMEOUT,
    CURVETYPE,
};
const int SETTING_ITEMS_PEDAL_CONTROL_SIZE = sizeof(settingItemsPedalControl) / sizeof(DrumSettingId);

#define SETTING_TO_JSON(label, ymlName)        \
  if (settingId == label) {                    \
    settingsNode[#ymlName] = settings.ymlName; \
    return;                                    \
  }

#define SETTING_TO_JSON_THRESHOLD(label, settingName, settingField) \
  if (settingId == label) {                                         \
    settingsNode[#settingName] = settings.settingField;             \
    return;                                                         \
  }

static String mapPadTypeToString(PadType value) {
  using enum PadType;
  MAP_ENUM_TO_STRING(Drum);
  MAP_ENUM_TO_STRING(Cymbal);
  MAP_ENUM_TO_STRING(Pedal);
  eventLog.log(Level::WARN, String("Unknown pad type '") + (uint8_t)value + "' -> using fallback");
  return mapPadTypeToString(PAD_TYPE_DEFAULT);
}

static String mapZonesTypeToString(ZonesType value) {
  using enum ZonesType;
  MAP_ENUM_TO_STRING(Zones1_Controller);
  MAP_ENUM_TO_STRING(Zones1_Piezo);
  MAP_ENUM_TO_STRING(Zones2_Piezos);
  MAP_ENUM_TO_STRING(Zones2_PiezoAndSwitch);
  MAP_ENUM_TO_STRING(Zones3_Piezos);
  MAP_ENUM_TO_STRING(Zones3_PiezoAndSwitches_2TRS);
  MAP_ENUM_TO_STRING(Zones3_PiezoAndSwitches_1TRS);
  eventLog.log(Level::WARN, String("Unknown zones type '") + (uint8_t)value + "' -> using fallback");
  return mapZonesTypeToString(ZONES_TYPE_DEFAULT);
}

static String mapChokeTypeToString(ChokeType value) {
  using enum ChokeType;
  MAP_ENUM_TO_STRING(None);
  MAP_ENUM_TO_STRING(Switch_Cup);
  MAP_ENUM_TO_STRING(Switch_Edge);
  eventLog.log(Level::WARN, String("Unknown choke type '") + (uint8_t)value + "' -> using fallback");
  return mapChokeTypeToString(None);
}

static String mapCurveTypeToString(CurveType value) {
  using enum CurveType;
  MAP_ENUM_TO_STRING(Linear);
  MAP_ENUM_TO_STRING(Exponential1);
  MAP_ENUM_TO_STRING(Exponential2);
  MAP_ENUM_TO_STRING(Logarithmic1);
  MAP_ENUM_TO_STRING(Logarithmic2);
  eventLog.log(Level::WARN, String("Unknown curve type '") + (uint8_t)value + "' -> using fallback");
  return mapCurveTypeToString(CURVE_TYPE_DEFAULT);
}

static void convertPadSettingToJson(DrumSettingId settingId, JsonObject settingsNode, const DrumSettings& settings) {
  if (settingId == PAD_TYPE) {
    settingsNode[SETTINGS_PAD_TYPE_PROP] = mapPadTypeToString(settings.padType);
    return;
  }

  if (settingId == ZONES_TYPE) {
    settingsNode[SETTINGS_ZONES_TYPE_PROP] = mapZonesTypeToString(settings.zonesType);
    return;
  }

  if (settingId == CHOKE_TYPE) {
    settingsNode[SETTINGS_CHOKE_TYPE_PROP] = mapChokeTypeToString(settings.chokeType);
    return;
  }

  if (settingId == CURVETYPE) {
    settingsNode[SETTINGS_CURVETYPE_PROP] = mapCurveTypeToString(settings.curveType);
    return;
  }

  if (settingId == ZONE_THRESHOLDS) {
    JsonArray zoneThresholdsNode = settingsNode["zoneThresholds"].to<JsonArray>();
    zone_size_t zones = settings.getZoneCount();
    for (zone_size_t zone = 0; zone < zones; ++zone) {
      zoneThresholdsNode[zone]["min"] = settings.zoneThresholdsMin[zone];
      if (settings.hasZoneThresholdMax(zone)) {
        zoneThresholdsNode[zone]["max"] = settings.zoneThresholdsMax[zone];
      }
    }
    return;
  }

  SETTING_TO_JSON(SCAN_TIME_MS, scanTimeUs)
  SETTING_TO_JSON(MASK_TIME_MS, maskTimeMs)
  SETTING_TO_JSON(ALMOST_CLOSED_THRESHOLD, almostClosedThreshold)
  SETTING_TO_JSON(CLOSED_THRESHOLD, closedThreshold)
  SETTING_TO_JSON_THRESHOLD(MOVE_DETECT_TOLERANCE, moveDetectTolerance, moveDetectTolerance)
  SETTING_TO_JSON(CHICK_DETECT_TIMEOUT, chickDetectTimeoutMs)
  SETTING_TO_JSON(HEAD_RIM_BIAS, headRimBias)
  SETTING_TO_JSON(ENABLE_CROSS_NOTE, crossNoteEnabled)
}

static int getSupportedSettings(const DrumPad& pad, DrumSettingId* supportedSettingIds) {
  switch (pad.getSettings().zonesType) {
  case ZonesType::Zones1_Controller:
    memcpy((void*)supportedSettingIds, settingItemsPedalControl, sizeof(settingItemsPedalControl));
    return SETTING_ITEMS_PEDAL_CONTROL_SIZE;
  case ZonesType::Zones1_Piezo:
  case ZonesType::Zones2_Piezos:
  case ZonesType::Zones3_Piezos: {
    size_t count = SETTING_ITEMS_PIEZO_ONLY_SIZE;
    memcpy((void*)supportedSettingIds, settingItemsPiezoOnly, sizeof(settingItemsPiezoOnly));
    if (pad.getPadType() == PadType::Drum) {
      if (pad.getSettings().getZoneCount() == 2) {
        supportedSettingIds[count++] = ENABLE_CROSS_NOTE;
      }
      if (pad.getSettings().getZoneCount() > 1) {
        supportedSettingIds[count++] = HEAD_RIM_BIAS;
      }
    }
    return count;
  }
  case ZonesType::Zones2_PiezoAndSwitch:
  case ZonesType::Zones3_PiezoAndSwitches_2TRS:
  case ZonesType::Zones3_PiezoAndSwitches_1TRS:
    memcpy((void*)supportedSettingIds, settingItemsPiezoAndSwitches, sizeof(settingItemsPiezoAndSwitches));
    return SETTING_ITEMS_PIEZO_AND_SWITCHES_SIZE;
  default: // always return the basic options to change type and disable
    memcpy((void*)supportedSettingIds, settingItemsFallback, sizeof(settingItemsFallback));
    return SETTING_ITEMS_FALLBACK_SIZE;
  }
}

void DrumConfigMapper::convertPadSettingsToJson(const DrumPad& pad, const DrumKit& drumKit, JsonObject& settingsNode) {
  const DrumSettings& padSettings = pad.getSettings();

  DrumSettingId supportedSettingIds[32] = {};
  int settingsCount = getSupportedSettings(pad, supportedSettingIds);

  for (int i = 0; i < settingsCount; ++i) {
    convertPadSettingToJson(supportedSettingIds[i], settingsNode, padSettings);
  }
}
