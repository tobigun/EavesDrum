// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#if __has_include(<tusb.h>)
#include <tusb.h>
#if !defined(ESP32) && (CFG_TUH_ENABLED > 0)
#define ENABLE_TINY_USB_HOST
#endif
#endif

#ifdef ENABLE_TINY_USB_HOST

#include <Arduino.h>

enum class UsbHostDeviceClass {
  Midi,
  Hid,
  None
};

class UsbHost {
public:
  static void begin();
  static void update();

  static String getVendorName(const tuh_itf_info_t& info);
  static String getProductName(const tuh_itf_info_t& info);
  static String getSerial(const tuh_itf_info_t& info);

  static String getConnectedDeviceName() { return connectedDeviceName; }
  static void updateConnectedDeviceName(const tuh_itf_info_t* info, UsbHostDeviceClass devClass = UsbHostDeviceClass::None);

private:
  static String connectedDeviceName;
};

#endif
