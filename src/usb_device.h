// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

class UsbDevice {
public:
  static void begin();
  static void end();

  static bool isInited();

  static void update();

  static uint16_t getVendorId();
  static void setVendorId(uint16_t vendorId);

  static uint16_t getProductId();
  static void setProductId(uint16_t vendorId);

  static void enableHid(const char* name, const uint8_t* hidDescriptor, uint16_t hidDescriptorSize, uint8_t hidPollInterval);
  static void disableHid();
};
