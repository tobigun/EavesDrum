// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_pad.h"
#include "latency.h"

void LatencySensing::sense(time_us_t senseTimeUs, LatencyTestInfo& info) {
  if (!pad.isConnectorActive()) {
    return;
  }

  resetState();

  pad.sensorValues[0] = pad.readInput(pad.connector->getPin(0), true);

  if (info.state != WAIT_FOR_HIT) {
    return; // do not update latency info with newer hit
  }

  if (pad.sensorValues[0] >= info.threshold) {
    pad.hits[0] = true;
    pad.hitTimeUs = senseTimeUs;
    if (!info.preview) {
      info.state = SENDING_RESULT;
    }
  }
}

void LatencySensing::resetState() {
  const zone_size_t zoneCount = pad.getActiveZoneCount();
  for (zone_size_t zone = 0; zone < zoneCount; ++zone) {
    pad.hits[zone] = false;
    pad.hitVelocities[zone] = 0;
    pad.sensorValues[zone] = 0;
  }
  pad.cymbal.isChoked = false;
}
