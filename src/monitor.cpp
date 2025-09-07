// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "monitor.h"

#include "drum_kit.h"
#include "log.h"
#include "midi_device.h"
#include "webui.h"
#include "drum/sensing/latency.h"

#define HISTORY_MIN_WRITE_INTERVAL_US 200

bool DrumMonitor::checkAndSendMonitoredPadHitInfo() {
  DrumPad* monitoredPad = getMonitoredPad();
  if (!monitoredPad) {
    return false;
  }

  addPadHistoryEntry(*monitoredPad);

  // Note: update historyHitIndex _after_ addPadHistoryEntry() as otherwise history.isFull() will
  // return true after a hit and the entry will not be written to the buffer
  if (isLatencyTestActive()) {
    return checkAndSendLatencyHitInfo(*monitoredPad);
  } else if (monitoredPad->getPadType() == PadType::Pedal) {
    return checkAndSendMonitoredPedalHitInfo(*monitoredPad);
  } else {
    return checkAndSendMonitoredDrumHitInfo(*monitoredPad);
  }
}

/**
 * Checks all non-monitored pads for hits (but only if monitoring is enabled)
 */
void DrumMonitor::checkAndSendNonMonitoredPadHitInfo() {
  if (!triggeredByAllPads) {
    return;
  }

  DrumPad* monitoredPad = getMonitoredPad();
  for (pad_size_t padIndex = 0; padIndex < drumKit->getPadsCount(); ++padIndex) {
    DrumPad* pad = drumKit->getPad(padIndex);
    bool isMonitored = pad == monitoredPad; // monitored pads are handled seoarately as they contain history data
    if (!isMonitored && pad->isHit()) {
      MonitorHitInfo hitInfo = (pad->getPadType() == PadType::Pedal)
          ? preparePedalHitInfo(*pad)
          : prepareDrumHitInfo(*pad);
      sendHitMessage(hitInfo, false);
    }
  }
}

bool DrumMonitor::checkAndSendMonitoredDrumHitInfo(const DrumPad& pad) {
  if (pad.getSensingState() == SensingState::Scan) {
    if (!history.isTriggerStartPosSet()) { // hit detected -> scan time started
      history.setTriggerStartPos();
    }
    // update scan index until State!=Scan. As a result, historyScanEndIndex will mark the end of the scan time
    history.setTriggerEndPos();
  }

  bool isHit = pad.isHit();
  if (isHit || pad.cymbal.isChoked || isExternallyTriggered) {
    if (!history.isTriggerStartPosSet()) {
      history.setTriggerStartPos();
    }

    monitoredPadHitInfo = prepareDrumHitInfo(pad);

    // wait until mask time is over to also show samples during mask time in UI
    nextHitSendTimeMs = millis() + pad.getSettings().maskTimeMs;
    isExternallyTriggered = false;
    return false;
  }

  return waitOrSendHitMessage(false);
}

bool DrumMonitor::checkAndSendMonitoredPedalHitInfo(const DrumPad& pedal) {
  // wait time until move settles to prevent that another event is triggered immediately after
  const time_ms_t waitTimeBeforeSendMs = 100;

  // Note that "writeIndex != HISTORY_INDEX_NONE" is always true as we immediately send when the buffer is full
  if (pedal.isHit() || isExternallyTriggered) {
    if (!history.isTriggerStartPosSet()) {
      history.setTriggerStartPos(); // no moving before -> mark hit as start
    }
    history.setTriggerEndPos(); // mark the chick or external trigger as the end of the move event
    monitoredPadHitInfo = preparePedalHitInfo(pedal);
    nextHitSendTimeMs = millis() + waitTimeBeforeSendMs;
    isExternallyTriggered = false;
  } else if (pedal.hihat.isMoving) {
    if (!nextHitSendTimeMs) { // no trigger event active -> mark beginning of move as start
      history.setTriggerStartPos();
      monitoredPadHitInfo = preparePedalHitInfo(pedal);
      nextHitSendTimeMs = millis() + waitTimeBeforeSendMs;
    } else { // update pedal CC to show latest valid CC value in UI
      monitoredPadHitInfo.velocities[0] = pedal.hihat.pedalCC;
    }
  }

  return waitOrSendHitMessage(true);
}

bool DrumMonitor::checkAndSendLatencyHitInfo(const DrumPad& pad) {
  if (pad.isHit() && !nextHitSendTimeMs) {
    history.setTriggerStartPos();
    monitoredPadHitInfo = prepareLatencyTestHitInfo(pad);

    // show a little bit of the signal after the hit also, but we also want as much data as possible
    // from the start of the latency test
    nextHitSendTimeMs = millis() + 3;
    return false;
  }

  return waitOrSendHitMessage(false);
}

bool DrumMonitor::waitOrSendHitMessage(bool sendIfHistoryFull) {
  bool sendPadHit = nextHitSendTimeMs && millis() >= nextHitSendTimeMs;

  if (sendPadHit || (sendIfHistoryFull && history.isFull())) {
    sendHitMessage(monitoredPadHitInfo, true);

    nextHitSendTimeMs = 0;
    history.resetTrigger();
    return true;
  }

  return false;
}

bool DrumMonitor::addPadHistoryEntry(const DrumPad& pad) {
  time_us_t currentTimeUs = micros();

  time_us_t timeUntilPreviousUs = currentTimeUs - lastHistoryUpdateTimeUs;
  if (timeUntilPreviousUs < HISTORY_MIN_WRITE_INTERVAL_US) {
    return false;
  }
  lastHistoryUpdateTimeUs = currentTimeUs;

  // add a gap if no sensor value was available for some time
  if (timeUntilPreviousUs > 2 * HISTORY_MIN_WRITE_INTERVAL_US) {
    history.addGapEntry(HISTORY_MIN_WRITE_INTERVAL_US);
    timeUntilPreviousUs -= HISTORY_MIN_WRITE_INTERVAL_US;
  }

  return history.addValueEntry(timeUntilPreviousUs, pad.sensorValues);
}

static inline MonitorHitInfo createGenericHitInfo(const DrumPad& pad, const DrumSettings& settings) {
  return {
      .padIndex = pad.getIndex(),
      .isChoked = pad.cymbal.isChoked,
      .padType = (enum_uint8_t)settings.padType,
      .zonesType = (enum_uint8_t)settings.zonesType,
      .chokeType = (enum_uint8_t)settings.chokeType,
  };
}

MonitorHitInfo DrumMonitor::prepareDrumHitInfo(const DrumPad& pad) {
  const DrumSettings& settings = pad.getSettings();
  MonitorHitInfo hitInfo = createGenericHitInfo(pad, settings);

  for (int i = 0; i < pad.getActiveZoneCount(); ++i) {
    hitInfo.hits[i] = pad.hits[i];
    hitInfo.velocities[i] = pad.hitVelocities[i];
    hitInfo.zoneThresholdsMin[i] = settings.zoneThresholdsMin[i];
    hitInfo.zoneThresholdsMax[i] = settings.zoneThresholdsMax[i];
  }

  return hitInfo;
}

MonitorHitInfo DrumMonitor::prepareLatencyTestHitInfo(const DrumPad& pad) {
  const DrumSettings& settings = pad.getSettings();
  MonitorHitInfo hitInfo = createGenericHitInfo(pad, settings);
  
  hitInfo.hits[0] = true;
  hitInfo.velocities[0] = 0;
  hitInfo.zoneThresholdsMin[0] = latencyTest.threshold;
  hitInfo.zoneThresholdsMax[0] = MAX_SENSOR_VALUE;

  // add latency info only in single trigger mode (i.e. in the final test, not in the preview)
  hitInfo.latencyUs = latencyTest.preview ? 0 : micros() - latencyTest.startTimeUs;

  return hitInfo;
}

MonitorHitInfo DrumMonitor::preparePedalHitInfo(const DrumPad& pedal) {
  const DrumSettings& settings = pedal.getSettings();
  MonitorHitInfo hitInfo = createGenericHitInfo(pedal, settings);

  hitInfo.hits[0] = true; // hit = pedal is moving -> always true
  hitInfo.velocities[0] = pedal.hihat.pedalCC;
  hitInfo.zoneThresholdsMin[0] = settings.zoneThresholdsMin[0];
  hitInfo.zoneThresholdsMax[0] = settings.zoneThresholdsMax[0];

  // chick detection
  hitInfo.hits[1] = pedal.hits[0];
  hitInfo.velocities[1] = pedal.hitVelocities[0];
  hitInfo.zoneThresholdsMin[1] = settings.almostClosedThreshold;
  hitInfo.zoneThresholdsMax[1] = settings.closedThreshold;

  return hitInfo;
}

void DrumMonitor::sendHitMessage(const MonitorHitInfo& hitInfo, bool includeHistoryData) {
  memcpy(&msgBuffer.hitInfo, &hitInfo, sizeof(hitInfo));

  size_t messageSize;
  if (includeHistoryData) {
    history.copyTo(msgBuffer.history);
    msgBuffer.hitInfo.triggerStartIndex = history.getRelativeTriggerStartPos();
    msgBuffer.hitInfo.triggerEndIndex = history.getRelativeTriggerEndPos();
    messageSize = sizeof(msgBuffer);
  } else {
    msgBuffer.hitInfo.triggerStartIndex = -1;
    msgBuffer.hitInfo.triggerEndIndex = -1;
    messageSize = sizeof(hitInfo);
  }

  webUI.sendBinaryToWebSocket((uint8_t*)&msgBuffer, messageSize);
}

void DrumMonitor::startLatencyTest(bool preview, sensor_value_t threshold, midi_note_t midiNote) {
  const DrumPad* pad = getMonitoredPad();
  if (!pad) {
    return;
  }

  latencyTest.startTimeUs = micros();
  latencyTest.state = WAIT_FOR_HIT;
  latencyTest.threshold = threshold;
  latencyTest.preview = preview;

  if (!preview) {
    drumKit->sendMidiNoteOnOffMessage(midiNote, 100);
  }
}

void DrumMonitor::updateLatencyTest(time_us_t senseTimeUs) {
  DrumPad* pad = getMonitoredPad();
    if (!pad) {
    return;
  }

  LatencySensing(*pad).sense(senseTimeUs, latencyTest);

  // update monitor with data until hit and mask time is over
  checkAndSendMonitoredPadHitInfo();
}
