// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef NO_MIDI

#include "midi_device.h"

void MidiDevice::begin() {}
void MidiDevice::sendNoteOn(DataByte inNoteNumber, DataByte inVelocity, Channel inChannel) {}
void MidiDevice::sendNoteOff(DataByte inNoteNumber, DataByte inVelocity, Channel inChannel) {}
void MidiDevice::sendAfterTouch(DataByte inPressure, Channel inChannel) {}
void MidiDevice::sendAfterTouch(DataByte inNoteNumber, DataByte inPressure, Channel inChannel) {}
void MidiDevice::sendControlChange(DataByte inControlNumber, DataByte inControlValue, Channel inChannel) {}

#endif
