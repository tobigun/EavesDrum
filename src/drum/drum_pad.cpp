// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_pad.h"

#include "event_log.h"
#include "sensing/controller.h"
#include "sensing/piezo.h"
#include "sensing/piezo_switch.h"

DrumMappings DrumPad::defaultMappings = {
  .role = "None",
  .name = "Default",
};

void DrumPad::init() {
  if (pedalPad && pedalPad->getPadType() != PadType::Pedal) {
    eventLog.log(Level::ERROR, String("Pad[") + pedalPad->getName() + "] does not have type 'Pedal'");
    pedalPad = nullptr;
  }
}

sensor_value_t DrumPad::readInput(DrumPin& pin, bool autoCalibrate) {
  sensor_value_t result = pin.mux
      ? pin.mux->readChannel(pin.index)
      : DrumIO::readAnalogInPin(pin.index);

  if (pin.getSignalType() == SignalType::VoltageOffset) {
    if (autoCalibrate) {
      sensor_value_t offset = pin.updateOffset(result);
      result = (result > offset)
        ? (uint32_t)(result - offset) * MAX_SENSOR_VALUE / (MAX_SENSOR_VALUE - offset)
        : (uint32_t)(offset - result) * MAX_SENSOR_VALUE / offset;
    } else {
      result = abs(result - MAX_SENSOR_VALUE / 2) * 2;
    }
  }

  if (pin.isInverted()) {
    result = (MAX_SENSOR_VALUE - result);
  }

  return result;
}

void DrumPad::sense(time_us_t senseTimeUs) {
  switch (settings.zonesType) {
  case ZonesType::Zones1_Piezo:
  case ZonesType::Zones2_Piezos:
  case ZonesType::Zones3_Piezos:
    PiezoSensing(*this).sense(senseTimeUs);
    break;
  case ZonesType::Zones2_PiezoAndSwitch:
  case ZonesType::Zones3_PiezoAndSwitches_1TRS:
  case ZonesType::Zones3_PiezoAndSwitches_2TRS:
    PiezoSwitchSensing(*this).sense(senseTimeUs);
    break;
  case ZonesType::Zones1_Controller:
    ControllerSensing(*this).sense(senseTimeUs);
    break;
  }
}
