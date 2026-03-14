// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi_transport_esp32_ble_midi.h"
#include "ble_server.h"

#include <BLEMidi.h>

BleServer bleServer;

void BleServer::updateServer(bool enabled) {}

void MidiTransport_Esp32BleMidi::begin() {
  //BLEMidiServer.setOnConnectCallback(onBluetoothConnected);
  //BLEMidiServer.setOnDisconnectCallback(onBluetoothDisconnected);
  BLEMidiServer.begin("EavesDrum BLE MIDI");
}

void MidiTransport_Esp32BleMidi::sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
  BLEMidiServer.noteOn(inChannel, inNoteNumber, inVelocity);
}

void MidiTransport_Esp32BleMidi::sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
  BLEMidiServer.noteOff(inChannel, inNoteNumber, inVelocity);
}

void MidiTransport_Esp32BleMidi::sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) {
  BLEMidiServer.afterTouch(inChannel, inPressure);
}

void MidiTransport_Esp32BleMidi::sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) {
  BLEMidiServer.afterTouchPoly(inChannel, inNoteNumber, inPressure);
}

void MidiTransport_Esp32BleMidi::sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) {
  BLEMidiServer.controlChange(inChannel, inControlNumber, inControlValue);
}
