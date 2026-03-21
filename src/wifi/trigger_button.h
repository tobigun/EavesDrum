// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

class TriggerButton {
public:
  void init();

  bool isPressed();
};

extern TriggerButton triggerButton;
