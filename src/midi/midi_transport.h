// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdint.h>
#include <vector>
#include "util.h"
#include "log.h"

enum class MidiOutputMode {
  UsbDevice,
  UsbHost,
  SerialDin,
  BleClient,
  BleServer,
  GuitarHeroDrum
};

inline String midiOutputModeToString(MidiOutputMode value) {
  using enum MidiOutputMode;
  MAP_ENUM_TO_STRING(UsbDevice);
  MAP_ENUM_TO_STRING(UsbHost);
  MAP_ENUM_TO_STRING(SerialDin);
  MAP_ENUM_TO_STRING(BleClient);
  MAP_ENUM_TO_STRING(BleServer);
  MAP_ENUM_TO_STRING(GuitarHeroDrum);
  return ENUM_TO_STRING(UsbDevice);
}

inline MidiOutputMode parseMidiOutputMode(String value) {
  using enum MidiOutputMode;
  MAP_STRING_TO_ENUM(UsbDevice);
  MAP_STRING_TO_ENUM(UsbHost);
  MAP_STRING_TO_ENUM(SerialDin);
  MAP_STRING_TO_ENUM(BleClient);
  MAP_STRING_TO_ENUM(BleServer);
  MAP_STRING_TO_ENUM(GuitarHeroDrum);
  return UsbDevice;
}

typedef uint8_t midi_channel_t;

class MidiTransport {
public:
  virtual void begin() = 0;

  virtual void shutdown() {}

  virtual void update() {}

  virtual void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) = 0;

  virtual void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) = 0;

  virtual void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) = 0;

  virtual void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) = 0;

  virtual void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) = 0;
};

struct MidiTransportInstances {
  MidiTransport* usbDevice = nullptr;
  MidiTransport* usbHost = nullptr;
  MidiTransport* serialDin = nullptr;
  MidiTransport* bleClient = nullptr;
  MidiTransport* bleServer = nullptr;
  MidiTransport* guitarHeroDrum = nullptr;
};

class MidiTransportMultiplexer : public MidiTransport {
public:
  MidiTransportMultiplexer(MidiTransportInstances& instances)
    : instances(instances),
      selectedTransport(instances.usbDevice) {}

  void setOutputMode(MidiOutputMode mode) {
    MidiTransport* newMidiTransport = getTransportInstance(mode);
    if (initialized && newMidiTransport != selectedTransport) {
      selectedTransport->shutdown();      
      newMidiTransport->begin();
    }
    selectedTransport = newMidiTransport;
  }
  
  std::vector<MidiOutputMode> getSupportedOutputModes() const {
    std::vector<MidiOutputMode> supportedModes;
    if (instances.usbDevice) {
      supportedModes.push_back(MidiOutputMode::UsbDevice);
    }
    if (instances.usbHost) {
      supportedModes.push_back(MidiOutputMode::UsbHost);
    }
    if (instances.serialDin) {
      supportedModes.push_back(MidiOutputMode::SerialDin);
    }
    if (instances.bleClient) {
      supportedModes.push_back(MidiOutputMode::BleClient);
    }
    if (instances.bleServer) {
      supportedModes.push_back(MidiOutputMode::BleServer);
    }
    if (instances.guitarHeroDrum) {
      supportedModes.push_back(MidiOutputMode::GuitarHeroDrum);
    }
    return supportedModes;
  }

  virtual void begin() {
    initialized = true;
    selectedTransport->begin();
  }

  virtual void shutdown() {
    initialized = false;
    selectedTransport->shutdown();
  }

  virtual void update() {
    selectedTransport->update();
  }

  virtual void sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
    selectedTransport->sendNoteOn(inNoteNumber, inVelocity, inChannel);
  }

  virtual void sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
    selectedTransport->sendNoteOff(inNoteNumber, inVelocity, inChannel);
  }

  virtual void sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) {
    selectedTransport->sendAfterTouch(inPressure, inChannel);
  }

  virtual void sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) {
    selectedTransport->sendAfterTouch(inNoteNumber, inPressure, inChannel);
  }

  virtual void sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) {
    selectedTransport->sendControlChange(inControlNumber, inControlValue, inChannel);
  }

private:
  MidiTransport* getTransportInstance(MidiOutputMode mode) {
    switch (mode) {
    case MidiOutputMode::UsbDevice:
      return instances.usbDevice;
    case MidiOutputMode::UsbHost:
      return instances.usbHost;
    case MidiOutputMode::SerialDin:
      return instances.serialDin;
    case MidiOutputMode::BleClient:
      return instances.bleClient;
    case MidiOutputMode::BleServer:
      return instances.bleServer;
    case MidiOutputMode::GuitarHeroDrum:
      return instances.guitarHeroDrum;
    default:
      return instances.usbDevice;
    }
  }

private:
  MidiTransportInstances& instances;
  MidiTransport* selectedTransport;
  bool initialized = false;
};

extern MidiTransportMultiplexer midiTransport;
