// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi_transport_portmidi.h"
#ifdef ENABLE_MIDI_PORTMIDI_TRANSPORT

#include "portmidi.h"
#include "porttime.h"

#include <Arduino.h>

// https://www.zem-college.de/midi/mc_cvm7.htm
#define NOTE_OFF 0x80
#define NOTE_ON 0x90
#define POLYPHON_PRESSURE 0xA0
#define CONTROL_CHANGE 0xB0
#define PROGRAM_CHANGE 0xC0
#define CHANNEL_PRESSURE 0xD0
#define PITCH_BEND 0xE0

// Windows does not support virtual MIDI devices. Use loopMIDI as output device instead
static const String MIDI_DEVICE_NAME = "loopMIDI Port";

static PortMidiStream* stream;

static PmDeviceID findDeviceId() {
  for (int id = 0; id < Pm_CountDevices(); id++) {
    const PmDeviceInfo* info = Pm_GetDeviceInfo(id);
    if (info->output && MIDI_DEVICE_NAME.equals(info->name)) {
      printf("Found MIDI out device: id=%d: if=%s, name=%s\n", id, info->interf, info->name);
      fflush(stdout);
      return id;
    }
  }
  return pmNoDevice;
}

static void printMidiError(PmError err) {
  if (err == pmHostError) {
    const int STRING_MAX = 80;
    char line[STRING_MAX];
    Pm_GetHostErrorText(line, STRING_MAX);
    printf("PortMidi host error: %s\n", line);
  } else if (err < 0) {
    printf("PortMidi error: %s\n", Pm_GetErrorText(err));
  }
  fflush(stdout);
}

void MidiTransport_PortMidi::start() {
  PmDeviceID deviceId = findDeviceId();

  Pt_Start(1, 0, 0);
  PmError err = Pm_OpenOutput(&stream, deviceId, NULL, 100, NULL, NULL, 0);
  if (err < 0) {
    printMidiError(err);
    return;
  }
}

#define STATUS(cmd, channel) ((cmd & 0xF0) + (channel & 0x0F))

void MidiTransport_PortMidi::sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
  Pm_WriteShort(stream, 0, Pm_Message(STATUS(NOTE_ON, inChannel), inNoteNumber, inVelocity));
}

void MidiTransport_PortMidi::sendNoteOff(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
  Pm_WriteShort(stream, 0, Pm_Message(STATUS(NOTE_OFF, inChannel), inNoteNumber, inVelocity));
}

void MidiTransport_PortMidi::sendAfterTouch(uint8_t inPressure, midi_channel_t inChannel) {
  Pm_WriteShort(stream, 0, Pm_Message(STATUS(CHANNEL_PRESSURE, inChannel), inPressure, 0));
}

void MidiTransport_PortMidi::sendAfterTouch(uint8_t inNoteNumber, uint8_t inPressure, midi_channel_t inChannel) {
  Pm_WriteShort(stream, 0, Pm_Message(STATUS(POLYPHON_PRESSURE, inChannel), inNoteNumber, inPressure));
}

void MidiTransport_PortMidi::sendControlChange(uint8_t inControlNumber, uint8_t inControlValue, midi_channel_t inChannel) {
  Pm_WriteShort(stream, 0, Pm_Message(STATUS(CONTROL_CHANGE, inChannel), inControlNumber, inControlValue));
}

#endif
