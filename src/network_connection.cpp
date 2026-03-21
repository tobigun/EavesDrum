// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "network_connection.h"

#include "drum_io.h"
#include "log.h"

NetworkConnection networkConnection;

NetworkUsb networkUsb;
#ifdef HAS_WIFI
NetworkWifi networkWifi;
#endif

Network *network = &networkUsb;

void selectNetwork() {
#ifdef HAS_WIFI
  bool useWifi = DrumIO::isButtonPressed(ButtonId::Wifi);
  if (useWifi) {
    logInfo("WiFi network selected");
    network = &networkWifi;
  } else {
    logInfo("USB network selected");
  }
#endif
}

void NetworkConnection::begin() {
  selectNetwork();
  network->begin();
}

void NetworkConnection::update() {
  network->update();
}
