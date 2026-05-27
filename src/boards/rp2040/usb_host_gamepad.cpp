// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "usb_host_gamepad.h"

#ifdef ENABLE_TINY_USB_HOST_GAMEPAD

#include "log.h"
#include "webui.h"
#include <hidparser.h>

#define GAMEDPAD_POLL_INTERVAL_MS 10

static bool enabled;

static uint8_t devAddress;
static uint8_t devIndex = TUSB_INDEX_INVALID_8;
HID_ReportInfo_t* hidReportInfo;

static ReportCallback reportCallback = nullptr;

static bool connectedDeviceNameDirty = false;

static void unmountDevice();

void UsbHostGamepad::start(ReportCallback callback) {
  reportCallback = callback;
  enabled = true;
  UsbHost::begin();
}

void UsbHostGamepad::stop() {
  reportCallback = nullptr;
  enabled = false;
  unmountDevice();
  UsbHost::updateConnectedDeviceName(nullptr);
}

static void updateConnectedDeviceInfo() {
  if (devIndex == TUSB_INDEX_INVALID_8) {
    UsbHost::updateConnectedDeviceName(nullptr);
    return;
  }

  tuh_itf_info_t info;
  if (!tuh_hid_itf_get_info(devAddress, devIndex, &info)) {
    logError("tuh_hid_itf_get_info failed");
  }

  UsbHost::updateConnectedDeviceName(&info, UsbHostDeviceClass::Hid);
}

void UsbHostGamepad::update() {
  UsbHost::update();
  if (connectedDeviceNameDirty) {
    updateConnectedDeviceInfo();
    connectedDeviceNameDirty = false;
  }

  if (devIndex != TUSB_INDEX_INVALID_8) {
    static uint32_t last_report_time_ms = 0;
    uint32_t now = millis();
    if (now - last_report_time_ms >= GAMEDPAD_POLL_INTERVAL_MS) {
      tuh_hid_receive_report(devAddress, devIndex);
      last_report_time_ms = now;
    }
  }
}

void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* report_desc, uint16_t desc_len) {
  if (!enabled) {
    return;
  }

  logDebug("USB-Host: HID Device Index = %u, HID device address = %u", idx, dev_addr);  

  if (devIndex != TUSB_INDEX_INVALID_8) { // no HID device is currently connected
    logError("A different USB HID Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled");
    return;
  }
  
  uint8_t protocol = tuh_hid_interface_protocol(dev_addr, idx);
  if (protocol != HID_ITF_PROTOCOL_NONE) {
    logError("Unsupported HID protocol: %d", protocol);
    return;
  }

  tuh_hid_report_info_t report_info;
  uint8_t report_count = tuh_hid_parse_report_descriptor(&report_info, 1, report_desc, desc_len);
  if (report_count == 0) {
    logError("No HID report report found");
    return;
  }

  if (report_info.usage_page != HID_USAGE_PAGE_DESKTOP) {
    logError("Unsupported HID usage page: %02X", report_info.usage_page);
    return;
  }

  if (report_info.usage != HID_USAGE_DESKTOP_GAMEPAD && report_info.usage != HID_USAGE_DESKTOP_JOYSTICK) {
    logError("Unsupported HID usage: 0x%02X", report_info.usage);
    return;
  }

  if (USB_ProcessHIDReport(report_desc, desc_len, &hidReportInfo) != HID_PARSE_Successful) {
    logError("Failed to parse HID report descriptor");
    return;
  }

#if 0
  logInfo("USB-Host: HID Items: %u %u", hidReportInfo->TotalReportItems, hidReportInfo->TotalDeviceReports);
  HID_ReportItem_t* reportItem = hidReportInfo->FirstReportItem;
  while (reportItem != hidReportInfo->LastReportItem) {
    logInfo("usage=%d %d", reportItem->Attributes.Usage.Page, reportItem->Attributes.Usage.Usage);
    reportItem = reportItem->Next;
  }
#endif

  devIndex = idx;
  devAddress = dev_addr;
  connectedDeviceNameDirty = true;
}

static void unmountDevice() {
    devIndex = TUSB_INDEX_INVALID_8;
    devAddress = 0;
    connectedDeviceNameDirty = true;
    if (hidReportInfo) {
      USB_FreeReportInfo(hidReportInfo);
      hidReportInfo = nullptr;
    }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t idx) {
  if (!enabled) {
    return;
  }

  if (dev_addr == devAddress && idx == devIndex) {
    unmountDevice();
    logInfo("USB-Host: HID Device Index = %u is unmounted", idx);
  } else {
    logInfo("USB-Host: Unused HID Device Index  %u is unmounted", idx);
  }
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t idx, uint8_t const* report, uint16_t len) {
  (void)dev_addr;
  (void)idx;
  (void)len;

  if (!enabled) {
    return;
  }

  if (hidReportInfo) {
    USB_Host_Data_t data;
    memset(&data, 0, sizeof(data));
    fill_generic_report(hidReportInfo, report, &data);

    logDebug("report: up=%d down=%d left=%d right=%d genericX=%d genericY=%d genericButtons=%d",
      data.dpadUp, data.dpadDown, data.dpadLeft, data.dpadRight, data.genericAxisX, data.genericAxisY, data.genericButtons);

    if (reportCallback) {
      reportCallback(data);
    }
  }
}

#endif
