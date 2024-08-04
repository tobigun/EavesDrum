// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#if 0

#include <BLEMidi.h>
#include "midi_device.h"

void MidiDevice::begin() {
  BLEMidiServer.setOnConnectCallback(onBluetoothConnected);
  BLEMidiServer.setOnDisconnectCallback(onBluetoothDisconnected);
  BLEMidiServer.begin("Lucies MIDI");
}

void MidiDevice::sendNoteOn(DataByte inNoteNumber, DataByte inVelocity, Channel inChannel) {
  BLEMidiServer.noteOn(inChannel, inNoteNumber, inVelocity);
}

void MidiDevice::sendNoteOff(DataByte inNoteNumber, DataByte inVelocity, Channel inChannel) {
  BLEMidiServer.noteOff(inChannel, inNoteNumber, inVelocity);
}

void MidiDevice::sendAfterTouch(DataByte inPressure, Channel inChannel) {
  BLEMidiServer.afterTouch(inChannel, inPressure);
}

void MidiDevice::sendAfterTouch(DataByte inNoteNumber, DataByte inPressure, Channel inChannel) {
  BLEMidiServer.afterTouchPoly(inChannel, inNoteNumber, inPressure);
}

void MidiDevice::sendControlChange(DataByte inControlNumber, DataByte inControlValue, Channel inChannel) {
  BLEMidiServer.controlChange(inChannel, inControlNumber, inControlValue);
}

#endif
