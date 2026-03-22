// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport.h"
#include "midi_transport_arduino_midi.h"
#include "midi_serial_usb_host.h"

class MidiTransport_UsbHost : public MidiTransport_ArduinoMidi<MidiSerialUsbHost> {
public:
  MidiTransport_UsbHost()
    : MidiTransport_ArduinoMidi<MidiSerialUsbHost>(midiSerialUsbHost) {}

  void begin() override;
  void update() override;
  void shutdown() override;

  String getConnectedDeviceName();

private:
  MidiSerialUsbHost midiSerialUsbHost;
};

extern MidiTransport_UsbHost midiTransportUsbHost;
