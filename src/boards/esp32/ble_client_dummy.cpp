// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ble_client.h"

BleClient bleClient;

BleClientStatus BleClient::getStatus() const {
  return BleClientStatus::Disconnected;
}

void BleClient::setPairingInfo(const String& name, const String& address) {}

const BleClientPairingInfo& BleClient::getPairingInfo() const {
  static BleClientPairingInfo pairingInfo;
  return pairingInfo;
}

bool BleClient::isScanning() const { return false; }

void BleClient::startDeviceScan() {}
