// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "config/config_mapper.h"
#include "drum_pad.h"
#include "drum_mux.h"
#include "monitor.h"
#include "note_event_queue.h"

#include <queue>

#define MAX_GATE_TIME_MS (30 * 1000) // 30 seconds

class DrumMonitor;

class DrumKit {
public:
  DrumKit() : drumMonitor(this) {
    for (pad_size_t padIndex = 0; padIndex < MAX_PAD_COUNT; ++padIndex) {
      pads[padIndex].setIndex(padIndex);
    }
  };

  // disable shallow copies
  DrumKit(const DrumKit&) = delete;
  DrumKit& operator=(const DrumKit&) = delete;

  // enable move semantic
  DrumKit(DrumKit&& other) = default;
  DrumKit& operator=(DrumKit&& other) = default;

  void init() {
    for (pad_size_t padIndex = 0; padIndex < padsCount; ++padIndex) {
      pads[padIndex].init();
    }
  }

  void updateDrums();
  
  // Pad

  DrumPad* getPad(pad_size_t index) { return (index < padsCount) ? &pads[index] : nullptr; }
  const DrumPad* getPad(pad_size_t index) const  { return (index < padsCount) ? &pads[index] : nullptr; }
  
  // get pad without bounds check (for internal use only)
  DrumPad* getPadUnchecked(pad_size_t index) { return &pads[index]; }

  DrumPad* getPadByName(String name) {
    for (pad_size_t index = 0; index < padsCount; ++index) {
      if (pads[index].getName() == name) {
        return &pads[index];
      }
    }
    return nullptr;
  }
  
  DrumPad* getPadByRole(String role) {
    for (pad_size_t index = 0; index < padsCount; ++index) {
      if (pads[index].getRole() == role) {
        return &pads[index];
      }
    }
    return nullptr;
  }

  pad_size_t getPadsCount() const { return padsCount; }

  DrumPad& addPad() {
    return pads[padsCount++];
  }

  // Mappings
  
  DrumMappings* getMappings(mappings_size_t index) { return index < mappingsCount ? &mappings[index] : nullptr; }
  
  const DrumMappings* getMappings(mappings_size_t index) const  { return index < mappingsCount ? &mappings[index] : nullptr; }
  
  DrumMappings* getMappings(String role) {
    for (mappings_size_t index = 0; index < mappingsCount; ++index) {
      if (mappings[index].role == role) {
        return &mappings[index];
      }
    }
    return nullptr;
  }

  // returns nullptr if the mappings could not be found and could not be created
  DrumMappings* getOrCreateMappings(String role) {
    DrumMappings* searchResult = getMappings(role);
    if (searchResult) {
      return searchResult;
    }

    if (mappingsCount >= MAX_MAPPINGS_COUNT) {
      eventLog.log(Level::ERROR, String("Cannot create mappings, maximum count reached: ") + MAX_MAPPINGS_COUNT);
      return nullptr;
    }
    
    DrumMappings* newMappings = &mappings[mappingsCount++];
    *newMappings = DrumMappings(role); // reset mappings to defaults as the slot might already have been used before
    return newMappings;
  }

  mappings_size_t getMappingsCount() const { return mappingsCount; }

  /**
   * Deletes all mappings and resets the mappings assigned to pads.
   */
  void deleteAllMappings() {
    // clear all mappings
    mappingsCount = 0;

    // reset mapping assigned to pads
    for (auto& pad : pads) {
      pad.setMappings(nullptr);
    }
  }

  // Mux

  DrumMux* getMux(mux_size_t index) { return index >= muxCount ? nullptr : &mux[index]; }
  const DrumMux* getMux(mux_size_t index) const { return const_cast<DrumKit*>(this)->getMux(index); }
  mux_size_t getMuxCount() const { return muxCount; }

  void addMux(DrumMux& newMux) {
    mux[muxCount] = std::move(newMux);
    muxCount++;
  }

  // Connector

  const DrumConnector* getConnector(connector_size_t index) const { return index >= connectorsCount ? nullptr : &connectors[index]; }
  mux_size_t getConnectorsCount() const { return connectorsCount; }
  
  DrumConnector* getConnectorById(ConnectorId id) {
    for (connector_size_t index = 0; index < connectorsCount; ++index) {
      if (connectors[index].getId() == id) {
        return &connectors[index];
      }
    }
    return nullptr;
  }

  void addConnector(DrumConnector& newConnector) {
    connectors[connectorsCount] = std::move(newConnector);
    connectorsCount++;
  }

  // Monitor

  DrumMonitor& getMonitor() { return drumMonitor; }
  const DrumMonitor& getMonitor() const { return drumMonitor; }

  // General

  time_ms_t getGateTime() const { return gateTimeMs; }

  /**
   * Sets the gate time in milliseconds.
   * 
   * Will be clamped to the range 0..MAX_GATE_TIME_MS.
   */
  void setGateTime(time_ms_t gateTimeMs) {
    if (gateTimeMs > MAX_GATE_TIME_MS) {
      eventLog.log(Level::WARN, String("Gate time reduced to maximum ")
        + MAX_GATE_TIME_MS + "ms (was " + String(gateTimeMs) + "ms)");
      gateTimeMs = MAX_GATE_TIME_MS;
    }
    this->gateTimeMs = gateTimeMs;
  }

public:
  void sendMidiNoteOnOffMessage(midi_note_t note, midi_velocity_t velocity);

private:
  void evaluatePad(time_us_t senseTimeUs, DrumPad& pad);
  void evaluateDrum(const DrumPad& pad);
  void evaluateHiHat(const DrumPad& pad, const DrumPad& pedal);
  void evaluateCymbal(const DrumPad& pad);
  void evaluateCymbalWithNotes(const DrumPad& pad, const midi_note_t* notes);
  
  void sendMidiNoteOnMessage(midi_note_t note, midi_velocity_t velocity);
  void sendPendingMidiNoteOffMessages();
  void sendMidiNoteOnWithDelayedOffMessage(midi_note_t note, midi_velocity_t velocity);

  void readMultiplexers(time_us_t senseTimeUs);
  void stabilizeMultiplexerOffsetVoltage(time_us_t senseTimeUs);
  void flushMultiplexers();

private:
  /**
   * The gate time is the time (in ms) between sending a NoteOff to a NoteOn event.
   * 
   * If the gate time is set to 0 the NoteOff event is sent immediately after the NoteOn event.
   * Usually this is fine for Drum software like EZdrummer, but some DAWs (like Ableton Live) will accept choke events only
   * if the note is still played (i.e. no NoteOff event was received).
   * In this case, the Gate time should be set to a value greater than 0, e.g. several seconds.
   * @see https://blog.abletondrummer.com/cymbal-choke-with-e-drums-and-ableton-live/
   */
  time_ms_t gateTimeMs = 0; // 0 .. MAX_GATE_TIME_MS
  NoteEventQueue pendingNotesQueue;
  
  mux_size_t muxCount = 0;
  DrumMux mux[MAX_MUX_COUNT];

  pad_size_t padsCount = 0;
  DrumPad pads[MAX_PAD_COUNT];

  mappings_size_t mappingsCount = 0;
  DrumMappings mappings[MAX_MAPPINGS_COUNT];

  pad_size_t connectorsCount = 0;
  DrumConnector connectors[MAX_CONNECTOR_COUNT];

  DrumMonitor drumMonitor;

  time_us_t lastHitTimeUs = 0;

public:
  struct {
    // number of sensor pollings in the last 30 seconds
    uint32_t updateCountPer30s = 0;
  } statistics;
};
