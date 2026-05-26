// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "usb_host.h"
#if defined(ENABLE_TINY_USB_HOST) && CFG_TUH_HID > 0
#define ENABLE_TINY_USB_HOST_GAMEPAD
#endif

#ifdef ENABLE_TINY_USB_HOST_GAMEPAD

#include <controller_reports.h>

typedef void (*ReportCallback)(const USB_Host_Data_t& data);

class UsbHostGamepad {
public:
  static void start(ReportCallback callback);
  static void stop();
  static void update();
};

#endif
