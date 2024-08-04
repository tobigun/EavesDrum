// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_Defs.h"

using MIDI_NAMESPACE::DataByte;
using MIDI_NAMESPACE::Channel;

class MidiDevice {
public:
  void begin();

  void sendNoteOn(DataByte inNoteNumber, DataByte inVelocity, Channel inChannel);

  void sendNoteOff(DataByte inNoteNumber, DataByte inVelocity, Channel inChannel);

  void sendAfterTouch(DataByte inPressure, Channel inChannel);

  void sendAfterTouch(DataByte inNoteNumber, DataByte inPressure, Channel inChannel);

  void sendControlChange(DataByte inControlNumber, DataByte inControlValue, Channel inChannel);

private:
  void* data;
};

extern void onBluetoothConnected();
extern void onBluetoothDisconnected();
extern MidiDevice MIDI;
