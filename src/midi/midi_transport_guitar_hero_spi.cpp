// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi_transport_guitar_hero_spi.h"

#ifdef ENABLE_MIDI_GUITAR_HERO_SPI_TRANSPORT

#include <SPI.h>
#include <SPISlave.h>
#include "drum_io.h"
#include "packed.h"
#include "guitar_hero_util.h"

// Protocol information:
// https://blog.laplante.io/2012/08/16/hack-a-guitar-hero-drumset-to-use-it-with-any-computer-over-usb-part2/
// https://github.com/Santroller/Santroller/blob/master/src/shared/main/wt_drum.cpp

#define PIN_RX 16
#define PIN_CS 17
#define PIN_SCK 18
#define PIN_TX 19

#define DRUM_SPI SPISlave

#define NOTE_CONTROLLER 100 // Note used to indicate CC Info (unused)

#define NOTE_ON 0x90
#define NOTE_OFF 0x80

#define MIDI_CHANNEL 9

struct MidiMessage {
  uint8_t cmd;
  uint8_t note;
  uint8_t velocity;
} ATTR_PACKED;

MidiMessage midiNoteBuffer[NUM_PADS];
uint8_t midiNoteBufferCount = 0;

static uint8_t pendingPadHits[NUM_PADS];

static void handleDataReceived(uint8_t* data, size_t len) {
  uint8_t cmd = data[0];
  if (cmd == 0xAA) {
    midiNoteBufferCount = 0;
    for (uint8_t padIndex = 0; padIndex < NUM_PADS; padIndex++) {
      uint8_t velocity = pendingPadHits[padIndex];
      if (velocity) {
        pendingPadHits[padIndex] = 0;

        MidiMessage& message = midiNoteBuffer[midiNoteBufferCount];
        message.cmd = NOTE_ON | MIDI_CHANNEL;
        message.note = padIndexToNote(padIndex);
        message.velocity = velocity;
        
        midiNoteBufferCount++;
      }
    }
    DRUM_SPI.setData(&midiNoteBufferCount, 1);
  } else if (cmd == 0x55 && midiNoteBufferCount > 0) {
    DRUM_SPI.setData((const uint8_t*) midiNoteBuffer, midiNoteBufferCount * sizeof(MidiMessage));
    midiNoteBufferCount = 0;
  }
}

void reinitializeSPI() {
  DRUM_SPI.end();

  SPISettings spiSettings(2000000, BitOrder::MSBFIRST, SPI_MODE1);
  DRUM_SPI.begin(spiSettings);

  static uint8_t preamble = 0xAA;
  DRUM_SPI.setData(&preamble, 1);
}

void MidiTransport_GuitarHero_SPI::start(MidiOutputMode mode) {
  DRUM_SPI.setRX(PIN_RX);
  DRUM_SPI.setCS(PIN_CS);
  DRUM_SPI.setSCK(PIN_SCK);
  DRUM_SPI.setTX(PIN_TX);
  DRUM_SPI.onDataRecv(&handleDataReceived);

  attachInterrupt(digitalPinToInterrupt(PIN_CS), []() {
    // flush buffer and reset state when slave is deselected
    reinitializeSPI();
  }, RISING);

  reinitializeSPI();

  DrumIO::led(LedId::MidiConnected, true);
} 

void MidiTransport_GuitarHero_SPI::stop() {
  detachInterrupt(digitalPinToInterrupt(PIN_CS));
  DRUM_SPI.end();

  DrumIO::led(LedId::MidiConnected, false);
}

void MidiTransport_GuitarHero_SPI::sendNoteOn(uint8_t inNoteNumber, uint8_t inVelocity, midi_channel_t inChannel) {
  int8_t padId = noteToPadId(inNoteNumber);
  if (padId >= 0) {
    pendingPadHits[padId] = inVelocity;
  }
}

#endif
