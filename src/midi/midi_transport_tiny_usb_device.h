// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if __has_include (<tusb.h>)
#include <tusb.h>
#if CFG_TUD_ENABLED > 0 && CFG_TUD_MIDI > 0
#define ENABLE_MIDI_USB_DEVICE
#endif
#endif

#ifdef ENABLE_MIDI_USB_DEVICE

#include "midi_transport_arduino_midi.h"
#include "drum_io.h"

class MidiTransport_UsbDevice : public MidiTransport_ArduinoMidi {
public:
  void begin() override {
    DrumIO::led(LedId::MidiConnected, true);
  }

  void stop() override {
    DrumIO::led(LedId::MidiConnected, false);
  }

  size_t write(uint8_t b) override {
    return tud_midi_stream_write(0, &b, 1);
  }
};

#endif
