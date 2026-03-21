// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport.h"
#include "midi_transport_arduino_midi.h"
#include "midi_serial_usb_device.h"
#include "drum_io.h"

class MidiTransport_UsbDevice : public MidiTransport_ArduinoMidi<MidiSerialUsbDevice> {
public:
  MidiTransport_UsbDevice()
    : MidiTransport_ArduinoMidi<MidiSerialUsbDevice>(midiSerialUsbDevice) {}

  virtual void begin() {
    DrumIO::led(LedId::MidiConnected, true);
  }

  virtual void shutdown() {
    DrumIO::led(LedId::MidiConnected, false);
  }

private:
  MidiSerialUsbDevice midiSerialUsbDevice;
};
