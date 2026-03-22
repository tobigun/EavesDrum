// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if __has_include(<tusb.h>)
#include <tusb.h>
#if CFG_TUH_ENABLED > 0 && CFG_TUH_MIDI > 0
#define ENABLE_MIDI_TINY_USB_HOST_TRANSPORT
#endif
#endif

#ifdef ENABLE_MIDI_TINY_USB_HOST_TRANSPORT

#include "midi_transport.h"
#include "midi_transport_arduino_midi.h"
#include <MIDI.h>

class MidiTransport_TinyUsbHost : public MidiTransport_ArduinoMidi {
public:
  void begin() override;
  void update() override;
  void stop() override;

  void endTransmission() override {
    tuh_midi_write_flush(getDeviceIndex());
  };

  size_t write(uint8_t b) override {
    if (!isConnected())
      return 0;
    return tuh_midi_stream_write(getDeviceIndex(), 0, &b, 1);
  }

  String getConnectedDeviceName();

private:
  bool isConnected() {
    return getDeviceIndex() != TUSB_INDEX_INVALID_8;
  }

  uint8_t getDeviceIndex();
};

extern MidiTransport_TinyUsbHost midiTransportTinyUsbHost;

#endif
