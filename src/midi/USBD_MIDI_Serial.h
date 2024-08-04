/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 hathach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <tusb.h>
#include "Stream.h"

// stripped down version of Adafruit_USBD_MIDI
class USBD_MIDI_Serial : public Stream {
public:
  USBD_MIDI_Serial() {}

  // for MIDI library
  bool begin(uint32_t baud) {
    (void)baud;
    return true;
  }

  // Stream interface to use with MIDI Library
  virtual int read(void) {
    uint8_t ch;
    return tud_midi_stream_read(&ch, 1) ? (int)ch : (-1);
  }

  virtual size_t write(uint8_t b) { return tud_midi_stream_write(0, &b, 1); }
  virtual int available(void) { return tud_midi_available(); }
  virtual int peek(void) { return -1; } // MIDI Library does not use peek
  virtual void flush(void) {} // MIDI Library does not use flush

  using Stream::write;

  // Raw MIDI USB packet interface.
  bool writePacket(const uint8_t packet[4])  { return tud_midi_packet_write(packet); }
  bool readPacket(uint8_t packet[4]) { return tud_midi_packet_read(packet); }
};
