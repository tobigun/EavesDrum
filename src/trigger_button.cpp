// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "trigger_button.h"

#ifdef TRIGGER_BUTTON_PIN

#include <Arduino.h>

TriggerButton triggerButton;

bool TriggerButton::isPressed() {
  static uint32_t lastTriggerPressed = 0;
  static bool lastPressed = false;

  bool pressed = (digitalRead(TRIGGER_BUTTON_PIN) == LOW);
  if (pressed == lastPressed) {
    return false;
  }

  lastPressed = pressed;
  return pressed;
}

void TriggerButton::init() {
  pinMode(TRIGGER_BUTTON_PIN, INPUT_PULLUP);
}

#else

bool TriggerButton::isPressed() {
  return false;
}

void TriggerButton::init() {
}

#endif