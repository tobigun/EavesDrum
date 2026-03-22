// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef HAS_BLUETOOTH

#include "midi_transport_portmidi.h"
#ifdef ENABLE_MIDI_PORTMIDI_TRANSPORT
#define MidiTransport_BleSimulation_Base MidiTransport_PortMidi
#else
#include "midi_transport_dummy.h"
#define MidiTransport_BleSimulation_Base MidiTransport_Dummy
#endif

class MidiTransport_BleSimulation : public MidiTransport_BleSimulation_Base {
public:
  void start() override;

  void stop() override;

  void update() override;
};

#endif
