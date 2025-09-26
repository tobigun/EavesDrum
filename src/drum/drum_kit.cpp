// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_kit.h"
#include "log.h"

#include "config/config_mapper.h"
#include "midi_device.h"
#include "monitor.h"

#define HIHAT_CC 4
#define MIDI_CHANNEL 10

#define HALF_MINUTE_IN_US (30 * 1000L * 1000L)

#define HIT_INDICATOR_DELAY_US 100000

void DrumKit::updateDrums() {
  static time_us_t lastStatisticsResetTimeUs = 0;
  static uint32_t updateCountPer30s = 0;
  
  time_us_t senseTimeUs = micros();

  if (senseTimeUs - lastHitTimeUs > HIT_INDICATOR_DELAY_US) {
    DrumIO::led(LED_HIT_INDICATOR, false);
  }

  if (senseTimeUs - lastStatisticsResetTimeUs > HALF_MINUTE_IN_US) {
    statistics.updateCountPer30s = updateCountPer30s;
    lastStatisticsResetTimeUs = senseTimeUs;
    updateCountPer30s = 0;
  }
  ++updateCountPer30s;

  sendPendingMidiNoteOffMessages();

  readMultiplexers(senseTimeUs);
  
  if (drumMonitor.isLatencyTestActive()) {
    drumMonitor.updateLatencyTest(senseTimeUs);
    return;
  }

  for (pad_size_t i = 0; i < padsCount; ++i) {
    DrumPad& pad = pads[i];
    if (pad.isEnabled()) {
      evaluatePad(senseTimeUs, pad);
    }
  }

  drumMonitor.checkAndSendMonitoredPadHitInfo();
  drumMonitor.checkAndSendNonMonitoredPadHitInfo();
}

void DrumKit::readMultiplexers(time_us_t senseTimeUs) {
#ifndef SIMULATE_IO
  stabilizeMultiplexerOffsetVoltage(senseTimeUs);
#endif

  for (mux_size_t i = 0; i < muxCount; ++i) {
    mux[i].scan();
  }
}

/**
 * If the multiplexers are not read for a longer time (e.g. because the main loop is busy with webui tasks),
 * the line capacitance of the multiplexer input pins can cause wrong readings as the voltage drops to 0V and
 * needs to be stabilized to the bias voltage first (~1.5V if no pad is hit or switch is pressed).
 * This should only be necessary if the UI is used and not in normal operation.
 */
void DrumKit::stabilizeMultiplexerOffsetVoltage(time_us_t senseTimeUs) {
  static time_us_t lastMuxReadTimeUs = 0;
  
  // Note: do not make this too short as the monitor messages take about 600-700us and we do not want them
  // to cause a flush as we might miss a hit then.
  // On the opposite side, a blockage of 5ms already caused false hit triggers. So values between 1-4ms should be fine.
  const time_us_t flushTimeUs = 2 * 1000; // 2ms seems to be a good compromise
  bool needsFlush = senseTimeUs - lastMuxReadTimeUs > flushTimeUs;
  if (!needsFlush) {
    lastMuxReadTimeUs = senseTimeUs;
    return;
  }

  SerialDebug.printf("Flush Mux: %d ms\n", senseTimeUs - lastMuxReadTimeUs);
  flushMultiplexers();
  lastMuxReadTimeUs = micros();
}

/**
 * This method reads the multiplexers for a short time to to establish the voltage bias on the input lines.
 * This will take longer the higher the capitance and resistance of a line is.
 */
void DrumKit::flushMultiplexers() {
  time_ms_t startTimeMs = millis();
  do {
    for (mux_size_t i = 0; i < muxCount; ++i) {
      mux[i].scan();
    }
  } while (millis() - startTimeMs < 100); // assume that 100ms is enough to stabilize the voltage
}

static void sendChokeMessage(const DrumPad& pad, const midi_note_t* notes) {
  for (int i = 0; i < pad.getActiveZoneCount(); ++i) {
    MIDI.sendAfterTouch(notes[i], 127, MIDI_CHANNEL);
  }
  for (int i = 0; i < pad.getActiveZoneCount(); ++i) {
    MIDI.sendAfterTouch(notes[i], 0, MIDI_CHANNEL);
  }
}

void DrumKit::evaluatePad(time_us_t senseTimeUs, DrumPad& pad) {
  switch (pad.getPadType()) {
  case PadType::Drum:
    pad.sense(senseTimeUs);
    evaluateDrum(pad);
    break;
  case PadType::Cymbal: {
    DrumPad* pedal = pad.getPedalPad();
    if (pedal) { // HiHat
      pedal->sense(senseTimeUs);
      pad.sense(senseTimeUs);
      evaluateHiHat(pad, *pedal);
    } else {
      pad.sense(senseTimeUs);
      evaluateCymbal(pad);
    }
    break;
  }
  case PadType::Pedal: // ignore, handled by HiHat
    break;
  }
}

void DrumKit::evaluateDrum(const DrumPad& pad) {
  const DrumMappings& mappings = pad.getMappings();

  if (pad.hits[0]) {
    EDRUM_DEBUG("%s hit main: %d\n", pad.getName().c_str(), pad.hitVelocities[0]);
    sendMidiNoteOnMessage(mappings.noteMain, pad.hitVelocities[0]);
  } else if (pad.hits[1]) {
    EDRUM_DEBUG("%s hit rim: %d\n", pad.getName().c_str(), pad.hitVelocities[1]);
    sendMidiNoteOnMessage(mappings.noteRim, pad.hitVelocities[1]);
  } else if (pad.hits[2]) {
    EDRUM_DEBUG("%s hit side rim / cross-stick: %d\n", pad.getName().c_str(), pad.hitVelocities[2]);
    sendMidiNoteOnMessage(mappings.noteCross, pad.hitVelocities[2]);
  }
}

void DrumKit::evaluateHiHat(const DrumPad& pad, const DrumPad& pedal) {
  if (pedal.hihat.isMoving) {
    MIDI.sendControlChange(HIHAT_CC, pedal.hihat.pedalCC, MIDI_CHANNEL);
  }

  const DrumMappings& padMappings = pad.getMappings();
  const DrumMappings& pedalMappings = pedal.getMappings();

  bool isClosed = pedal.hihat.state == HiHatState::Closed;
  if (isClosed && pedalMappings.closedNotesEnabled == true) {
    const midi_note_t notes[] = {padMappings.noteCloseMain, padMappings.noteCloseRim, padMappings.noteCloseCup};
    evaluateCymbalWithNotes(pad, notes);
  } else {
    evaluateCymbal(pad);
  }

  if (pedalMappings.noteMain != MIDI_NOTE_UNASSIGNED && pedal.hits[0]) { // play chick sound when hihat is closed
    sendMidiNoteOnMessage(pedalMappings.noteMain, pedal.hitVelocities[0]);
  }
}

void DrumKit::evaluateCymbal(const DrumPad& pad) {
  const DrumMappings& padMappings = pad.getMappings();
  const midi_note_t notes[] = {padMappings.noteMain, padMappings.noteRim, padMappings.noteCup};
  evaluateCymbalWithNotes(pad, notes);
}

void DrumKit::evaluateCymbalWithNotes(const DrumPad& pad, const midi_note_t* notes) {
  if (pad.hits[0]) { // bow
    EDRUM_DEBUG("%s hit bow: %d\n", pad.getName().c_str(), pad.hitVelocities[0]);
    sendMidiNoteOnMessage(notes[0], pad.hitVelocities[0]);
  } else if (pad.hits[1]) { // edge
    EDRUM_DEBUG("%s hit edge: %d\n", pad.getName().c_str(), pad.hitVelocities[1]);
    sendMidiNoteOnMessage(notes[1], pad.hitVelocities[1]);
  } else if (pad.hits[2]) { // cup
    EDRUM_DEBUG("%s hit cup: %d\n", pad.getName().c_str(), pad.hitVelocities[2]);
    sendMidiNoteOnMessage(notes[2], pad.hitVelocities[2]);
  } else if (pad.cymbal.isChoked) {
    EDRUM_DEBUG("%s choked\n", pad.getName().c_str());
    sendChokeMessage(pad, notes);
  }
}

void DrumKit::sendMidiNoteOnOffMessage(midi_note_t note, midi_velocity_t velocity) {
  if (note != MIDI_NOTE_UNASSIGNED) {
    MIDI.sendNoteOn(note, velocity, MIDI_CHANNEL);
    MIDI.sendNoteOff(note, 0, MIDI_CHANNEL);
  }
}

void DrumKit::sendMidiNoteOnWithDelayedOffMessage(midi_note_t note, midi_velocity_t velocity) {
  if (pendingNotesQueue.removeNote(note)) { // stop note if it is already playing
    MIDI.sendNoteOff(note, 0, MIDI_CHANNEL);
  }

  MIDI.sendNoteOn(note, velocity, MIDI_CHANNEL);

  if (pendingNotesQueue.isFull()) { // remove oldest note if queue is full
    const MidiNoteEvent& oldNote = pendingNotesQueue.peekOldestNote();
    MIDI.sendNoteOff(oldNote.note, 0, MIDI_CHANNEL);
    pendingNotesQueue.removeOldestNote();
  }

  pendingNotesQueue.addNote(note, millis());
}

void DrumKit::sendMidiNoteOnMessage(midi_note_t note, midi_velocity_t velocity) {
  DrumIO::led(LED_HIT_INDICATOR, true);
  lastHitTimeUs = micros();

  if (note == MIDI_NOTE_UNASSIGNED) {
    return;
  }

  if (gateTimeMs > 0) {
    sendMidiNoteOnWithDelayedOffMessage(note, velocity);
  } else {
    sendMidiNoteOnOffMessage(note, velocity);
  }
}

void DrumKit::sendPendingMidiNoteOffMessages() {
  time_ms_t currentTimeMs = millis();
  while (!pendingNotesQueue.isEmpty()) {
    const MidiNoteEvent& noteEvent = pendingNotesQueue.peekOldestNote();
    if (currentTimeMs - noteEvent.noteOnTimeMs < gateTimeMs) {
      return; // wait until next check
    }

    MIDI.sendNoteOff(noteEvent.note, 0, MIDI_CHANNEL);
    pendingNotesQueue.removeOldestNote();
  } 
}
