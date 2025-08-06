// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include <MIDI.h>
#include <tusb.h>

#ifdef CFG_TUD_MIDI
#ifdef ESP32
#include <USB.h>
#include <USBMIDI.h>
#include <esp32-hal-tinyusb.h>
#endif
#include "midi/USBD_MIDI_Serial.h"
#else
#include <BLEMIDI_Transport.h>
#include <hardware/BLEMIDI_ESP32.h>
#endif
#include "midi_device.h"

#if CFG_TUD_MIDI
USBD_MIDI_Serial usb_midi;
MIDI_CREATE_INSTANCE(USBD_MIDI_Serial, usb_midi, MIDI_INSTANCE);
#else
BLEMIDI_CREATE_INSTANCE("Lucies MIDI", MIDI_INSTANCE);
#endif

#ifdef ESP32
extern "C" uint16_t tusb_drum_midi_load_descriptor(uint8_t *dst, uint8_t *itf) {
  uint8_t str_index = tinyusb_add_string_descriptor("TinyUSB MIDI");
  uint8_t ep_num = tinyusb_get_free_duplex_endpoint();
  TU_VERIFY(ep_num != 0);
  uint8_t descriptor[TUD_MIDI_DESC_LEN] = {
      // Interface number, string index, EP Out & EP In address, EP size
      TUD_MIDI_DESCRIPTOR(*itf, str_index, ep_num, (uint8_t)(0x80 | ep_num),
                          64)};
  *itf += 1;
  memcpy(dst, descriptor, TUD_MIDI_DESC_LEN);
  return TUD_MIDI_DESC_LEN;
}
#endif

void MidiDevice::begin() {
#ifndef CFG_TUD_MIDI
  BLEMIDI_INSTANCE.setHandleConnected(onBluetoothConnected);
  BLEMIDI_INSTANCE.setHandleDisconnected(onBluetoothDisconnected);
#endif

#ifdef ESP32
 tinyusb_enable_interface(USB_INTERFACE_MIDI, TUD_MIDI_DESC_LEN, tusb_drum_midi_load_descriptor);
#endif
 
  // Initialize MIDI, and listen to all MIDI channels
  // This will also call usb_midi's begin()
  MIDI_INSTANCE.begin(MIDI_CHANNEL_OMNI);
}

void MidiDevice::sendNoteOn(DataByte inNoteNumber, DataByte inVelocity, Channel inChannel) {
  MIDI_INSTANCE.sendNoteOn(inNoteNumber, inVelocity, inChannel);
}

void MidiDevice::sendNoteOff(DataByte inNoteNumber, DataByte inVelocity, Channel inChannel) {
  MIDI_INSTANCE.sendNoteOff(inNoteNumber, inVelocity, inChannel);
}

void MidiDevice::sendAfterTouch(DataByte inPressure, Channel inChannel) {
  MIDI_INSTANCE.sendAfterTouch(inPressure, inChannel);
}

void MidiDevice::sendAfterTouch(DataByte inNoteNumber, DataByte inPressure, Channel inChannel) {
  MIDI_INSTANCE.sendAfterTouch(inNoteNumber, inPressure, inChannel);
}

void MidiDevice::sendControlChange(DataByte inControlNumber, DataByte inControlValue, Channel inChannel) {
  MIDI_INSTANCE.sendControlChange(inControlNumber, inControlValue, inChannel);
}
