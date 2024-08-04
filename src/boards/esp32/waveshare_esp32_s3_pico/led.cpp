// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "led.h"
#include <Adafruit_NeoPixel.h>

#define PIN 21
#define NUMPIXELS 1

#define DIM_FACTOR 20

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGB + NEO_KHZ800);

void LED::begin() {
  strip.begin();
  strip.show();
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void LED::setColor(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, r / DIM_FACTOR, g / DIM_FACTOR, b / DIM_FACTOR);
  strip.show();
}
