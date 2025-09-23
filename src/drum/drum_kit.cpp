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

  if (senseTimeUs - last_hit_time_us > HIT_INDICATOR_DELAY_US) {
    DrumIO::led(LED_HIT_INDICATOR, false);
  }

  if (senseTimeUs - lastStatisticsResetTimeUs > HALF_MINUTE_IN_US) {
    statistics.updateCountPer30s = updateCountPer30s;
    lastStatisticsResetTimeUs = senseTimeUs;
    updateCountPer30s = 0;
  }
  ++updateCountPer30s;

  sendPendingMidiNoteOffMessages();

  for (mux_size_t i = 0; i < muxCount; ++i) {
    mux[i].scan();
  }
  
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
  last_hit_time_us = micros();

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
