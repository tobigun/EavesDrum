// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef SUPPORT_WIFI_CLIENT
#include <WiFiProvisioner.h>
#include <WebServer.h>
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#ifdef ARDUINO_ARCH_ESP32
#include <ESPmDNS.h>
#endif
#include "log.h"
#include "drum_io.h"

#include "network_connection.h"

#ifdef SUPPORT_WIFI_CLIENT
static WiFiProvisioner::WiFiProvisioner provisioner;
static WebServer *webServer;
#endif

static DNSServer *dnsServer;
static const byte DNS_PORT = 53;
static IPAddress apIP(192, 168, 4, 1);
static IPAddress netMsk(255, 255, 255, 0);

#define HOSTNAME "eaves"
#define WIFI_AP_NAME "EavesDrum"

enum class WifiMode {
  AccessPoint,
  Client
};
static WifiMode wifiMode = WifiMode::AccessPoint;

static void startAccessPoint() {
#ifdef SUPPORT_WIFI_CLIENT
  webServer = new WebServer(80); // for provisioning
  //WiFi.begin();
#endif

  dnsServer = new DNSServer();

  WiFi.mode(WIFI_AP_STA);
  delay(100);

  // Configure the access point
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(WIFI_AP_NAME);
  WiFi.setHostname(HOSTNAME);
  delay(100);

  IPAddress ip = WiFi.softAPIP();
  logString(Level::Info, "AP IP address: " + ip.toString());

  // Set up the DNS server
  dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer->start(DNS_PORT, "*", apIP);  
}

static void startClient() {
#ifdef SUPPORT_WIFI_CLIENT
  //provisioner.resetCredentials();
  provisioner.enableSerialDebug(true);
  
  provisioner.AP_NAME = WIFI_AP_NAME;
  //provisioner.SVG_LOGO = DRUM_SVG;

  provisioner.setConnectionTimeout(10000); // 10s
  provisioner.connectToWiFi();
  MDNS.begin(HOSTNAME);
}

void WifiConnect::provision() {
  provisioner.setupAccessPointAndServer();
  WiFi.setHostname(HOSTNAME);
#endif
}

static void updateClient() {
#if defined(SUPPORT_WIFI_CLIENT)
  if (triggerButton.isPressed()) {
    DrumIO::led(LedId::Network, false);
    wifi.provision();
    DrumIO::led(LedId::Network, true);
  }
#endif
}

void NetworkWifi::begin() {
  DrumIO::led(LedId::Network, false);
  if (wifiMode == WifiMode::AccessPoint) {
    startAccessPoint();
  } else {
    startClient();
  }
  DrumIO::led(LedId::Network, true);
}

void NetworkWifi::update() {
  if (wifiMode == WifiMode::Client) {
    updateClient();
  }
}
