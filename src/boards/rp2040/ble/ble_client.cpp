// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <btstack.h>

#include "ble_client.h"

#include "ble_midi_client.h"
#include "log.h"
#include "drum_io.h"
#include "webui.h"

#define CLIENT_PROFILE_NAME "EavesDrum BLE MIDI"

BleClient bleClient;

static BleClientPairingInfo pairingInfo;

static BleClientStatus clientStatus = BleClientStatus::Disconnected;

static uint32_t scanStartTimeMs = 0;
static volatile bool scanResultAvailable = false;
static uint64_t lastScanResultHash = 0;
static Ble_Scan_Result_t scanResults[BLEMC_MAX_SCAN_ITEMS];

#define IS_SCANNING() (scanStartTimeMs > 0)

static void stopDeviceScan();
static void updateConnectionStatus();
  
static void startClient() {
  ble_midi_client_init(CLIENT_PROFILE_NAME, strlen(CLIENT_PROFILE_NAME),
      IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
      SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
}

static void stopClient() {
  stopDeviceScan();
  ble_midi_client_deinit();
  while (ble_midi_client_is_connected()) {
    delay(100);
  }
  updateConnectionStatus();
}

static void reconnect() {
  bd_addr_t bdaddr;
  if (sscanf_bd_addr(pairingInfo.address.c_str(), bdaddr) != 1) {
    SerialDebug.printf("Invalid Bluetooth address: %s\n", pairingInfo.address.c_str());
    return;
  }

  ble_midi_client_set_last_connected(BD_ADDR_TYPE_LE_RANDOM, bdaddr);
  if (!ble_midi_client_waiting_for_connection()) {
    ble_midi_client_request_connect(0);
  }
}

BleClientStatus BleClient::getStatus() const {
  return clientStatus;
}

void BleClient::setPairingInfo(const String& name, const String& address) {
  pairingInfo.name = name;
  pairingInfo.address = address;

  stopDeviceScan();
  if (ble_midi_client_is_connected()) {
    ble_midi_client_request_disconnect();
  }
}

const BleClientPairingInfo& BleClient::getPairingInfo() const {
  return pairingInfo;
}

static uint64_t bdaddrToInt(bd_addr_t& bdaddr) {
  uint64_t result = 0;
  for (uint8_t i = 0; i < BD_ADDR_LEN; i++) {
    result += ((uint64_t) bdaddr[i]) << (8 * i);
  }
  return result;
}

bool BleClient::isScanning() const {
  return IS_SCANNING();
}

void BleClient::startDeviceScan() {
  SerialDebug.println("Start scanning for BLE devices...");
  ble_midi_client_set_scan_callback([]() {
    scanResultAvailable = true;
  });

  lastScanResultHash = 0;
  scanStartTimeMs = millis();
  ble_midi_client_scan_begin();
}

static void stopDeviceScan() {
  ble_midi_client_scan_end();
  scanStartTimeMs = 0;
}

static void updateScanStatus(uint32_t& scanStartTimeMs) {
  uint32_t currentTimeMs = millis();
  if (scanStartTimeMs) {
    if(currentTimeMs - scanStartTimeMs > CLIENT_SCAN_TIMEOUT_MS) {
      scanStartTimeMs = 0;
      stopDeviceScan();
    }
  }
}

static void sendScanResultsToWebUI() {
  uint8_t resultCount = BLEMC_MAX_SCAN_ITEMS;
  ble_midi_client_get_midi_peripherals(scanResults, &resultCount);

  uint64_t scanResultHash = 0;
  for (uint8_t i = 0; i < resultCount; i++) {
    uint64_t bdAddrInt = bdaddrToInt(scanResults[i].bdaddr);
    scanResultHash += bdAddrInt;
  }

  // do not send device list if nothing has changed
  if (lastScanResultHash == scanResultHash) {
    return;
  }
  lastScanResultHash = scanResultHash;

  std::vector<BleDeviceInfo> scannedDevices;
  for (uint8_t i = 0; i < resultCount; i++) {
    scannedDevices.push_back(BleDeviceInfo {
        String(scanResults[i].name),
        String(bd_addr_to_str(scanResults[i].bdaddr))
    });
  }
  webUI.sendBleScanResult(scannedDevices);
}

static void onConnectionChanged(BleClientStatus status, bool isScanning) {
  SerialDebug.printf("BLE client state changed: %s, scannning: %d\n",
    status == BleClientStatus::Connected ? "connected"
      : (status == BleClientStatus::Connecting ? "connecting"
      : "disconnected"),
    isScanning);
  DrumIO::led(LedId::Ble, status == BleClientStatus::Connected);
  webUI.sendBleStatus(status, isScanning);
}

static void updateConnectionStatus() {
  bool isConnected = ble_midi_client_is_connected();
  bool isConnecting = ble_midi_client_waiting_for_connection();
  BleClientStatus newStatus = isConnected ? BleClientStatus::Connected
    : (isConnecting ? BleClientStatus::Connecting
    : BleClientStatus::Disconnected);

  static bool wasScanning = false;
  bool scanning = IS_SCANNING();
  if (clientStatus != newStatus || scanning != wasScanning) {
    clientStatus = newStatus;
    wasScanning = scanning;
    onConnectionChanged(newStatus, scanning);
  }

  static bool blinkState = false;
  if (clientStatus == BleClientStatus::Connecting) {
    // waiting for connection, blink the LED
    DrumIO::led(LedId::Ble, blinkState);
    blinkState = !blinkState;
  }
}

static void updateEnabledState(bool enabled) {
  static bool wasEnabled = false;
  if (enabled == wasEnabled) {
    return;
  }

  wasEnabled = enabled;
  if (enabled) {
    startClient();
  } else {
    stopClient();
  }
}

void BleClient::updateClient(bool enabled) {
  updateEnabledState(enabled);
  if (!enabled) {
    return;
  }

  updateScanStatus(scanStartTimeMs);

  uint32_t currentTimeMs = millis();

  static uint32_t lastSyncTimeMs = 0;
  if (currentTimeMs - lastSyncTimeMs > 1000) {
    lastSyncTimeMs = currentTimeMs;

    updateConnectionStatus();
    
    if (isScanning() && scanResultAvailable) {
      sendScanResultsToWebUI();
      scanResultAvailable = false;
    }
  }

  static uint32_t lastConnectionCheckTime = 0;
  if (!isScanning() && currentTimeMs - lastConnectionCheckTime > 1000) {
    lastConnectionCheckTime = currentTimeMs;
    if (!pairingInfo.address.isEmpty() && !ble_midi_client_is_connected()) {
      reconnect();
    }
  }
}
