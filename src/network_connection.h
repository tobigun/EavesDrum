// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

class NetworkConnection {
public:
  void begin();
  void update();
};

extern NetworkConnection networkConnection;

class Network {
public:
  virtual void begin() = 0;
  virtual void update() = 0;
};

class NetworkUsb : public Network {
public:
  void begin() override;
  void update() override;
};

#ifdef HAS_WIFI
class NetworkWifi : public Network {
public:
  void begin() override;
  void update() override;
};
#endif