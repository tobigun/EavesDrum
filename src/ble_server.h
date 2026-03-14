// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

class BleServer {
public:
  void updateServer(bool enabled);
};

extern BleServer bleServer;
