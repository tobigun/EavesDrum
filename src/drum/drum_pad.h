// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "drum_connector.h"
#include "drum_mappings.h"
#include "drum_mux.h"
#include "drum_settings.h"
#include "event_log.h"
#include "types.h"
#include "sensing/sensing.h"

#define STRINGIFY(s) #s
#define STRINGIFY_VALUE(s) STRINGIFY(s)

// whether the sensor delivers positive and negative half waves or only the positive ones.
#define SIGNED_SENSOR_DATA
#define MAX_SENSOR_VALUE_STR STRINGIFY_VALUE(MAX_SENSOR_VALUE)

#define UNKNOWN_PAD 0xFF

enum class HiHatState {
  Open,
  AlmostClosed,
  Closed
};

enum class LastCymbalEventType {
  None,
  Hit,
  Choked
};

class DrumKit;

class DrumPad {
public:
  DrumPad() = default;

  // disable shallow copies
  DrumPad(const DrumPad&) = delete;
  DrumPad& operator=(const DrumPad&) = delete;

  // enable move semantic
  DrumPad(DrumPad&& other) = default;
  DrumPad& operator=(DrumPad&& other) = default;

public:
  void init();

  pad_size_t getIndex() const { return index; }

private:
  void setIndex(pad_size_t index) { this->index = index; }
  friend DrumKit;

public:
  const String& getName() const { return name; }
  void setName(String name) { this->name = name; }

  const String& getRole() const { return role; }
  void setRole(String role) { this->role = role; }

  bool isEnabled() const { return enabled; }
  void setEnabled(bool enabled) { this->enabled = enabled; }

  PadType getPadType() const { return settings.padType; }

  DrumConnector* getConnector() const { return connector; }
  void setConnector(DrumConnector* connector) { this->connector = connector; }

  bool isConnectorActive() const {
    return connector && connector->getPinCount() > 0;
  }

  /**
   * Returns the number of active zones, i.e. the number of zones that are currently connected and usable.
   * 
   * Usually the active zone count equals the number of pins that are available on the connector of the drum trigger module.
   * Here an example for a cymbal with 3 zones and 2x TRS connectors:
   * - 3 active zones if both TRS connectors are connected to the trigger module
   * - 2 active zones if only one TRS connector is connected
   * - 1 active zone if only the tip (T) pin is connected
   * - 0 active zones if not connected
   * 
   * But there are also multiplexed cymbals that combine 3 signals in one TRS connector (e.g. Yamaha-style cymbals):
   * - 3 active zones if all pins (T+R) of the TRS connector are connected to the trigger module
   * - 1 active zone if only the tip (T) pin is connected (as both switches are connected to the R pin)
   * - 0 active zones if not connected
   */
  zone_size_t getActiveZoneCount() const {
    pin_size_t activePinCount = getActivePinCount();
    if (settings.zonesType == ZonesType::Zones3_PiezoAndSwitches_1TRS) {
      return activePinCount == 2 ? 3 : activePinCount;
    }
    return activePinCount;
  }

  /**
   * Returns the number of active input pins, i.e. the number of pins that are currently connected to the pad.
   * 
   * Examples:
   * - a cymbal with 3 zones and 2x TRS connectors can have 3..0 pins (depending on whether it is connected via two TRS, one TRS, one TS or no jack).
   * - a cymbal with 3 zones multiplexed in one TRS connector can have 2..0 pins (depending on whether it is connected via TRS, TS or no jack).
   */
  pin_size_t getActivePinCount() const {
    if (!connector) {
      return 0;
    }

    pin_size_t pinCount = connector->getPinCount();
    return (settings.zonesType == ZonesType::Zones3_PiezoAndSwitches_1TRS)
      ? min(pinCount, 2) // special handling of TRS 3-zone cymbals
      : min(pinCount, settings.getZoneCount()); // otherwise zone count equals pin count
  }

  const String& getGroup() const { return group; }
  void setGroup(String name) { this->group = name; }

  const bool getAutoCalibrate() const { return autoCalibrate; }
  void setAutoCalibrate(bool autoCalibrate) { this->autoCalibrate = autoCalibrate; }

  DrumSettings& getSettings() { return settings; }
  const DrumSettings& getSettings() const { return settings; }

  DrumMappings& getMappings() { return mappings; }
  const DrumMappings& getMappings() const { return mappings; }

  void setCurve(CurveType curveType) { settings.curveType = curveType; }

  DrumPad* getPedalPad() const { return pedalPad; }
  void setPedalPad(DrumPad& pad) { this->pedalPad = &pad; }
  void setPedalPad(DrumPad* pad) { this->pedalPad = pad; }

  SensingState getSensingState() const { return sensingState; }

  bool isHit() const {
    return hits[0] || hits[1] || hits[2];
  }

  bool operator==(const DrumPad& other) const { return this == &other; }
  bool operator!=(const DrumPad& other) const { return this != &other; }

public:
  void sense(time_us_t senseTimeUs);

private:
  /**
   * Reads the current input value from the pin and converts the value if the pin has a voltage offset.
   * 
   * If the pin has a voltage offset, values below the corresponded value of the offset are considered to represent negative voltages.
   * As the sign of the piezo signal does not matter, the sign is removed to make calculations easier.
   * The offset will be removed and the values will be scaled to the range 0..1023.
   * Examples for 10 bit ADC values for the range [-1.5V .. 1.5V]: 0 (-1.5V) -> 1023, 511 (0V) -> 0, 1023 (1.5V) -> 1023
   * 
   * If the pin does not have a voltage offset, no conversion will take place.
   * Examples for 10 bit ADC values for the range [0V .. 1.5V]: 0 (0V) -> 0, 1023 (1.5V) -> 1023
   * 
   * If autoCalibrate is true the internal offset of the pin will be adjusted.
   */
  static sensor_value_t readInput(DrumPin& pin, bool autoCalibrate = false);

public: // state
  struct HiHatData {
    midi_velocity_t pedalCC = 0;
    HiHatState state = HiHatState::Open;
    bool isMoving = false;
    time_ms_t almostClosedTimeMs;
  } hihat;

  struct CymbalData {
    bool isChoked = false;
    LastCymbalEventType lastEventType = LastCymbalEventType::None;
  } cymbal;

  time_us_t hitTimeUs = 0;
  time_us_t scanTimeEndUs = 0;

  sensor_value_t sensorValues[3] = {0, 0, 0};
  sensor_value_t maxZoneValues[3] = {0, 0, 0};
  midi_velocity_t hitVelocities[3] = {0, 0, 0};
  bool hits[3] = {false, false, false};

private:
  pad_size_t index = UNKNOWN_PAD;
  String name;
  String role;
  String group;
  bool autoCalibrate = false;

  bool enabled = false;

  DrumSettings settings;
  DrumMappings mappings;

  DrumPad* pedalPad = nullptr;
  DrumConnector* connector = nullptr;

  SensingState sensingState = SensingState::PeakDetect;

private:
  friend class ControllerSensing;
  friend class PiezoSwitchSensing;
  friend class LatencySensing;
  friend class PiezoSensing;
};
