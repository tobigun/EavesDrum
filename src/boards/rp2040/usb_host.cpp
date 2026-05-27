// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "usb_host.h"

#ifdef ENABLE_TINY_USB_HOST

#include "drum_kit.h"
#include "log.h"
#include "pio_usb.h"
#include "webui.h"

#include <Arduino.h>
#include <tusb.h>

#define PIO_USB_DP_PIN 6 // default: use pins 6 (D+) and 7 (D-) on dedicated USB connector
#define PIO_USB_DP_PIN_BOARD_V1_1 16 // use pins 16 (D+) and 17 (D-) on 50-pin expansion port. Pins 6+7 are blocked by mux address pins

String UsbHost::connectedDeviceName;

int getFreeDmaChannelForPioUsb() {
  // pio usb wants a fixed dma channel. Lower channels are unlikely free so query for an unused channel
  int dmaChannel = dma_claim_unused_channel(true);
  if (dmaChannel < 0) {
    return -1;
  }
  dma_channel_unclaim(dmaChannel);
  return dmaChannel;
}

// search a free PIO that has free state machines for all three programs.
int getFreePio() {
  for (int pioIndex = NUM_PIOS - 1; pioIndex > 0; --pioIndex) {
    PIO pio = pio_get_instance(pioIndex);
    if (pio_sm_is_claimed(pio, 0) || pio_sm_is_claimed(pio, 1) || pio_sm_is_claimed(pio, 2) || pio_sm_is_claimed(pio, 3)) {
      continue;
    }
    return pioIndex;
  }
  return -1;
}

void UsbHost::begin() {
  if (tuh_inited()) {
    return;
  }

  int dmaChannelTx = getFreeDmaChannelForPioUsb();
  if (dmaChannelTx < 0) {
    eventLog.log(Level::Error, "No free DMA channel available for PIO USB Host");
    return;
  }

  int pioIndex = getFreePio();
  if (pioIndex < 0) {
    eventLog.log(Level::Error, "No free PIO available for PIO USB Host");
    return;
  }

  pio_usb_configuration_t pio_cfg = {
      (uint8_t)(drumKit.getBoardVersion() == BoardVersion::V1_1 ? PIO_USB_DP_PIN_BOARD_V1_1 : PIO_USB_DP_PIN),
      (uint8_t)pioIndex, // TX PIO
      PIO_SM_USB_TX_DEFAULT,
      (uint8_t)dmaChannelTx,
      (uint8_t)pioIndex, // RX PIO (use one PIO for RX and TX)
      PIO_SM_USB_RX_DEFAULT,
      PIO_SM_USB_EOP_DEFAULT,
      NULL,
      PIO_USB_DEBUG_PIN_NONE,
      PIO_USB_DEBUG_PIN_NONE,
      false,
      PIO_USB_PINOUT_DPDM};

  tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
  tuh_init(BOARD_TUH_RHPORT);
}

void UsbHost::update() {
  tuh_task_ext(0, false);
}

static String descriptorToString(uint16_t* buffer) {
  String value;
  uint8_t len = ((*buffer) & 0xFF) / 2;
  for (uint8_t idx = 1; idx < len; idx++) {
    value += String((char)buffer[idx]);
  }
  return value;
}

String UsbHost::getVendorName(const tuh_itf_info_t& info) {
  uint16_t buffer[128];
  if (tuh_descriptor_get_string_langid_sync(info.daddr, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
    uint16_t langid = buffer[1];
    if (tuh_descriptor_get_manufacturer_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      return descriptorToString(buffer);
    }
  }
  return "";
}

String UsbHost::getProductName(const tuh_itf_info_t& info) {
  uint16_t buffer[128];
  if (tuh_descriptor_get_string_langid_sync(info.daddr, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
    uint16_t langid = buffer[1];
    if (tuh_descriptor_get_product_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      return descriptorToString(buffer);
    }
  }
  return "";
}

String UsbHost::getSerial(const tuh_itf_info_t& info) {
  uint16_t buffer[128];
  if (tuh_descriptor_get_string_langid_sync(info.daddr, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
    uint16_t langid = buffer[1];
    if (tuh_descriptor_get_serial_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      return descriptorToString(buffer);
    }
  }
  return "";
}

static void printConnectedUsbDeviceName(const String& vendorName, const String& productName, UsbHostDeviceClass devClass) {
  logString(Level::Info, String("USB-Host ")
    + ((devClass == UsbHostDeviceClass::Midi) ? "MIDI" : "HID")
    + " device found:", LogMode::NoNewline);
  if (vendorName.length() > 0) {
    logString(Level::Info, " Vendor=\"", LogMode::NoPrefixOrNewline);
    logString(Level::Info, vendorName, LogMode::NoPrefixOrNewline);
    logString(Level::Info, "\"", LogMode::NoPrefixOrNewline);
  }
  if (productName.length() > 0) {
    logString(Level::Info, " Product=\"", LogMode::NoPrefixOrNewline);
    logString(Level::Info, productName, LogMode::NoPrefixOrNewline);
    logString(Level::Info, "\"", LogMode::NoPrefixOrNewline);
  }
  logString(Level::Info, "\n", LogMode::NoPrefixOrNewline);
}

void UsbHost::updateConnectedDeviceName(const tuh_itf_info_t* info, UsbHostDeviceClass devClass) {
  if (!info) {
    connectedDeviceName = "";
    webUI.sendUsbHostStatus(connectedDeviceName);
    return;
  }

  // Note: Interact MorayPad gamepad uses an invalid vendor string-ID that makes TinyUSB crash
  // -> use product name only for HID devices.
  String vendorName = (devClass == UsbHostDeviceClass::Midi) ? UsbHost::getVendorName(*info) : "";
  String productName = UsbHost::getProductName(*info);
  vendorName.trim();
  productName.trim();

  printConnectedUsbDeviceName(vendorName, productName, devClass);

  if (productName.length() > 0 && vendorName.length() > 0) {
    connectedDeviceName = vendorName + " " + productName;
  } else if (productName.length() > 0) {
    connectedDeviceName = productName;
  } else if (vendorName.length() > 0) {
    connectedDeviceName = vendorName;
  } else {
    connectedDeviceName = "";
  }

  webUI.sendUsbHostStatus(connectedDeviceName);
}

#endif
