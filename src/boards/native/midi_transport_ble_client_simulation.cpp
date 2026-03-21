// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi_transport_ble_client_simulation.h"

#include "log.h"
#include "webui.h"

#include <vector>

BleClient bleClient;

static bool isConnected = false;
static BleClientStatus clientStatus = BleClientStatus::Disconnected;

static BleClientPairingInfo pairingInfo;
static uint32_t clientScanStartTimeMs = 0;

#define IS_SCANNING() (clientScanStartTimeMs > 0)

static void sendStatusToWebUi();

static void startClient() {
  if (pairingInfo.address.isEmpty()) {
    isConnected = false;
  } else {
    isConnected = true;
  }
  sendStatusToWebUi();
} 

static void stopClient() {
  isConnected = false;
  clientScanStartTimeMs = 0;
  sendStatusToWebUi();
} 

static void sendStatusToWebUi() {
  webUI.sendBleStatus(
    isConnected ? BleClientStatus::Connected : BleClientStatus::Disconnected,
    IS_SCANNING());
}

static void sendScanResultsToWebUI() {
  std::vector<BleDeviceInfo> scannedDevices;
  scannedDevices.push_back(BleDeviceInfo {
    String("My Device"),
    String("00:11:22:33:44:55")
  });
  webUI.sendBleScanResult(scannedDevices);
}

BleClientStatus BleClient::getStatus() const {
  return clientStatus;
}

void BleClient::setPairingInfo(const String& name, const String& address) {
  pairingInfo.name = name;
  pairingInfo.address = address;

  startClient();
}

const BleClientPairingInfo& BleClient::getPairingInfo() const {
  return pairingInfo;
}

bool BleClient::isScanning() const {
  return IS_SCANNING();
}

void BleClient::startDeviceScan() {
  clientScanStartTimeMs = millis();
  sendStatusToWebUi();
  sendScanResultsToWebUI();
}

static void updateScanStatus(uint32_t& clientScanStartTimeMs) {
  uint32_t currentTimeMs = millis();
  if (clientScanStartTimeMs) {
    if(currentTimeMs - clientScanStartTimeMs > CLIENT_SCAN_TIMEOUT_MS) {
      clientScanStartTimeMs = 0;
      sendStatusToWebUi();
    }
  }
}

void MidiTransport_BleSimulation::begin(){
  startClient();
}

void MidiTransport_BleSimulation::shutdown(){
  stopClient();
}

void MidiTransport_BleSimulation::update(){
  updateScanStatus(clientScanStartTimeMs);
}
