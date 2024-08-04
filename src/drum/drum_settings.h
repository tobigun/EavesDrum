// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "types.h"

enum class PadType {
  Drum,
  Cymbal,
  Pedal
};

/**
 * Number and kind of zone sensors of the pad.
 * Examples:
 * - a snare with main and rim area has two zones
 * - a kick drum has one zone
 * - a pedal always has one zone (the position), the chick trigger (if enabled) is shared with this zone
 */
enum class ZonesType {
  // 1ZoneSwitch, // TODO
  Zones1_Controller,
  Zones1_Piezo,
  Zones2_Piezos,
  Zones2_PiezoAndSwitch,
  Zones3_Piezos,
  Zones3_PiezoAndSwitches_2TRS, // 3 zones on two TRS connectors, e.g Roland cymbals
  Zones3_PiezoAndSwitches_1TRS // 3 zones on one TRS connectors, (shared pin for edge and cup), e.g. Yamaha cymbals
};


enum class ChokeType {
  None,
  Switch_Edge, // for Cymbals: Rim switch
  Switch_Cup, // for Cymbals: Cup switch
  // TouchSensor // TODO: needs additional pin
};

enum class CurveType {
  Linear,
  Exponential1,
  Exponential2,
  Logarithmic1,
  Logarithmic2
};

#define PAD_TYPE_DEFAULT PadType::Drum
#define ZONES_TYPE_DEFAULT ZonesType::Zones1_Piezo
#define CHOKE_TYPE_DEFAULT ChokeType::None
#define CURVE_TYPE_DEFAULT CurveType::Linear

#define THRESHOLD_MIN_DEFAULT (MAX_SENSOR_VALUE / 2)
#define THRESHOLD_MAX_DEFAULT MAX_SENSOR_VALUE

struct DrumSettings {
  PadType padType = PAD_TYPE_DEFAULT;
  ZonesType zonesType = ZONES_TYPE_DEFAULT;
  ChokeType chokeType = CHOKE_TYPE_DEFAULT;

  CurveType curveType = CURVE_TYPE_DEFAULT;
  sensor_value_t zoneThresholdsMin[3] = {THRESHOLD_MIN_DEFAULT, THRESHOLD_MIN_DEFAULT, THRESHOLD_MIN_DEFAULT};
  sensor_value_t zoneThresholdsMax[3] = {THRESHOLD_MAX_DEFAULT, THRESHOLD_MAX_DEFAULT, THRESHOLD_MAX_DEFAULT};
  uint16_t scanTimeUs = 3; // drum, cymbal
  uint8_t maskTimeMs = 30; // drum, cymbal

  int8_t headRimBias = 0; // -100 .. 100
  bool crossNoteEnabled = false;

  // pedal
  float almostClosedThreshold = 90.f; // relative % to min-/max-range
  float closedThreshold = 100.f; // relative % to min-/max-range
  sensor_value_t moveDetectTolerance = 50;
  uint8_t chickDetectTimeoutMs = 20;

public:
  zone_size_t getZoneCount() const {
    switch (zonesType) {
    case ZonesType::Zones2_Piezos:
    case ZonesType::Zones2_PiezoAndSwitch:
      return 2;
    case ZonesType::Zones3_Piezos:
    case ZonesType::Zones3_PiezoAndSwitches_1TRS:
    case ZonesType::Zones3_PiezoAndSwitches_2TRS:
      return 3;
    default:
      return 1;
    }
  }

  bool hasZoneThresholdMax(zone_size_t zone) const {
    return zone == 0
      || zonesType == ZonesType::Zones2_Piezos
      || zonesType == ZonesType::Zones3_Piezos;
  }

  zone_size_t getRequiredPinCount() const {
    // zone count equals pin count except for the 2-pin 3-zone cymbals
    return (zonesType == ZonesType::Zones3_PiezoAndSwitches_1TRS) ? 2 : getZoneCount();
  }
};
