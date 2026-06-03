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
  GuitarHeroDrumSPI,
  GuitarHeroDrumWii,
  RockbandDrum,
  GamepadUsb
};

inline String midiOutputModeToString(MidiOutputMode value) {
  using enum MidiOutputMode;
  MAP_ENUM_TO_STRING(UsbDevice);
  MAP_ENUM_TO_STRING(UsbHost);
  MAP_ENUM_TO_STRING(SerialDin);
  MAP_ENUM_TO_STRING(BleClient);
  MAP_ENUM_TO_STRING(BleServer);
  MAP_ENUM_TO_STRING(GuitarHeroDrumSPI);
  MAP_ENUM_TO_STRING(GuitarHeroDrumWii);
  MAP_ENUM_TO_STRING(RockbandDrum);
  MAP_ENUM_TO_STRING(GamepadUsb);
  return ENUM_TO_STRING(UsbDevice);
}

inline MidiOutputMode parseMidiOutputMode(String value) {
  using enum MidiOutputMode;
  MAP_STRING_TO_ENUM(UsbDevice);
  MAP_STRING_TO_ENUM(UsbHost);
  MAP_STRING_TO_ENUM(SerialDin);
  MAP_STRING_TO_ENUM(BleClient);
  MAP_STRING_TO_ENUM(BleServer);
  MAP_STRING_TO_ENUM(GuitarHeroDrumSPI);
  MAP_STRING_TO_ENUM(GuitarHeroDrumWii);
  MAP_STRING_TO_ENUM(RockbandDrum);
  MAP_STRING_TO_ENUM(GamepadUsb);
  return UsbDevice;
}

typedef uint8_t midi_channel_t;

class MidiTransport {
public:
  virtual void start(MidiOutputMode mode) = 0;

  virtual void stop() {}

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
  MidiTransport* guitarHeroDrumWii = nullptr;
  MidiTransport* guitarHeroDrumSPI = nullptr;
  MidiTransport* rockbandDrum = nullptr;
  MidiTransport* gamepad = nullptr;
};

class MidiTransportMultiplexer : public MidiTransport {
public:
  MidiTransportMultiplexer(MidiTransportInstances& instances)
    : instances(instances),
      selectedMode(MidiOutputMode::UsbDevice),
      selectedTransport(instances.usbDevice) {}

  void setOutputMode(MidiOutputMode mode) {
    if (mode == selectedMode) {
      return;
    }

    MidiTransport* newMidiTransport = getTransportInstance(mode);
    if (initialized) {
      selectedTransport->stop();      
      newMidiTransport->start(mode);
    }
    selectedMode = mode;
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
    if (instances.guitarHeroDrumSPI) {
      supportedModes.push_back(MidiOutputMode::GuitarHeroDrumSPI);
    }
    if (instances.guitarHeroDrumWii) {
      supportedModes.push_back(MidiOutputMode::GuitarHeroDrumWii);
    }
    if (instances.rockbandDrum) {
      supportedModes.push_back(MidiOutputMode::RockbandDrum);
    }
    if (instances.gamepad) {
      supportedModes.push_back(MidiOutputMode::GamepadUsb);
    }
    return supportedModes;
  }

  void start() {
    initialized = true;
    selectedTransport->start(selectedMode);
  }

  void start(MidiOutputMode mode) override {
    bool wasInitialized = initialized;
    setOutputMode(mode);
    if (!wasInitialized) {
      start();
    }
  }

  void stop() override {
    initialized = false;
    selectedTransport->stop();
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
    case MidiOutputMode::GuitarHeroDrumSPI:
      return instances.guitarHeroDrumSPI;
    case MidiOutputMode::GuitarHeroDrumWii:
      return instances.guitarHeroDrumWii;
    case MidiOutputMode::RockbandDrum:
      return instances.rockbandDrum;
    case MidiOutputMode::GamepadUsb:
      return instances.gamepad;
    default:
      return instances.usbDevice;
    }
  }

private:
  MidiTransportInstances& instances;
  MidiOutputMode selectedMode;
  MidiTransport* selectedTransport;
  bool initialized = false;
};

extern MidiTransportMultiplexer midiTransport;
