// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef USE_WIFI

#include <WiFiProvisioner.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "wifi_connect.h"
#include "images/drum.h"

WiFiProvisioner::WiFiProvisioner provisioner;

WebServer *m_server;
DNSServer *m_dns_server;
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

void WifiConnect::startServer() {
  // Initialize the server object
  //m_server = new WebServer(80);
  m_dns_server = new DNSServer();

  // Set up the WiFi mode
  //WiFi.begin();
  WiFi.mode(WIFI_AP_STA);
  delay(100);

  // Configure the access point
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP("Lucie");
  WiFi.setHostname("trommel");
  delay(100);
  Serial.println("AP IP address: " + String(WiFi.softAPIP()));

  // Set up the DNS server
  m_dns_server->setErrorReplyCode(DNSReplyCode::NoError);
  m_dns_server->start(DNS_PORT, "*", apIP);  
}

void WifiConnect::connect() {
  //provisioner.resetCredentials();
  provisioner.enableSerialDebug(true);
  
  provisioner.AP_NAME = "Lucies Trommel";
  provisioner.SVG_LOGO = DRUM_SVG;

  provisioner.setConnectionTimeout(10000);
  provisioner.connectToWiFi();
  MDNS.begin("trommel");
}

void WifiConnect::provision() {
  provisioner.setupAccessPointAndServer();
  WiFi.setHostname("trommel");
}

#endif