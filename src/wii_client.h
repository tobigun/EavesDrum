// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "midi_transport_guitar_hero_wii.h"

#ifdef ENABLE_MIDI_GUITAR_HERO_WII_TRANSPORT

#include <Arduino.h>

struct WiiClientPairingInfo {
  String address;
  String linkKey;
};

class WiiClient {
public:
  void setPairingInfo(const String& address, const String& linkKey);
  const WiiClientPairingInfo& getPairingInfo() const;
};

extern WiiClient wiiClient;

#endif
