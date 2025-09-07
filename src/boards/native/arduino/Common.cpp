// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Common.h"

#include <sys/time.h>
#include <thread>

SerialDummy Serial;

void interrupts() {}
void noInterrupts() {}

/* C++ prototypes */
long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint16_t makeWord(uint16_t w) {
  return w;
}
uint16_t makeWord(uint8_t h, uint8_t l) {
  return (h << 8) | l;
}

time_ms_t millis() {
  return micros() / 1000l;
}

time_us_t micros() {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  return tv.tv_sec * 1000000l + tv.tv_usec;
}

void yield(void) { std::this_thread::yield(); }

void pinMode(pin_size_t pinNumber, PinMode pinMode) {}

void delay(unsigned long ms) {
  usleep(ms * 1000);
}