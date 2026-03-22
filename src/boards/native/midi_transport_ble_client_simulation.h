// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifdef ENABLE_MIDI_PORTMIDI_TRANSPORT

#include "midi_transport_portmidi.h"

class MidiTransport_BleSimulation : public MidiTransport_PortMidi {
public:
  void start() override;

  void shutdown() override;

  void update() override;
};

#endif
