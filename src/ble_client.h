// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <Arduino.h>
#include <functional>

#define CLIENT_SCAN_TIMEOUT_MS 30000

struct BleClientPairingInfo {
  String name;
  String address;
};

enum class BleClientStatus {
  Disconnected,
  Connecting,
  Connected
};

class BleClient {
public:
  void updateClient(bool enabled);

  BleClientStatus getStatus() const;

  void startDeviceScan();
  bool isScanning() const;

  void setPairingInfo(const String& name, const String& address);
  const BleClientPairingInfo& getPairingInfo() const;
};

extern BleClient bleClient;
