/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 hathach for Adafruit Industries
 * Copyright (c) 2025 Tobias Gunkel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "midi_transport_tiny_usb_host.h"

#ifdef ENABLE_MIDI_TINY_USB_HOST_TRANSPORT

#include <tusb.h>
#include "log.h"
#include "drum_io.h"
#include "webui.h"
#include "drum_kit.h"
#include "usb_host.h"

static uint8_t devIndex = TUSB_INDEX_INVALID_8;

static bool connectedDeviceNameDirty = false;
static String connectedDeviceName;

static void updateConnectedDeviceInfo();

void MidiTransport_TinyUsbHost::begin() {
  DrumIO::led(LedId::MidiConnected, false);

  UsbHost::begin();
}

void MidiTransport_TinyUsbHost::update() {
  UsbHost::update();
  if (connectedDeviceNameDirty) {
    updateConnectedDeviceInfo();
    connectedDeviceNameDirty = false;
  }
}

void MidiTransport_TinyUsbHost::stop() {
  // Pico PIO does not release its resources (PIO, DMA channels, alarm pool timers) on deinit.
  // As we can also not release the resources from here (especially not the alarm pool as we do not have access to the pointer),
  // we won't call tuh_deinit here. This avoids crashes when switching USB Host on and off.
  //tuh_deinit(BOARD_TUH_RHPORT);
}

String MidiTransport_TinyUsbHost::getConnectedDeviceName() {
  return connectedDeviceName;
}

uint8_t MidiTransport_TinyUsbHost::getDeviceIndex() {
  return devIndex;
}


//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

#ifdef __cplusplus
extern "C"
{
  void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* mount_cb_data);
  void tuh_midi_umount_cb(uint8_t idx);
  void tuh_midi_rx_cb(uint8_t idx, uint32_t num_bytes);
  void tuh_midi_tx_cb(uint8_t idx, uint32_t num_bytes);
}
#endif

// Invoked when device with MIDI interface is mounted
void tuh_midi_mount_cb(uint8_t idx, const tuh_midi_mount_cb_t* mount_cb_data) {
  logInfo("USB-Host: MIDI Device Index = %u, MIDI device address = %u, %u IN cables, OUT %u cables", idx,
      mount_cb_data->daddr, mount_cb_data->rx_cable_count, mount_cb_data->tx_cable_count);

  if (devIndex == TUSB_INDEX_INVALID_8) {
    // then no MIDI device is currently connected
    devIndex = idx;
    connectedDeviceNameDirty = true;
  } else {
    logError("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled");
  }
  DrumIO::led(LedId::MidiConnected, true);
}

// Invoked when device with MIDI interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx) {
  if (idx == devIndex) {
    devIndex = TUSB_INDEX_INVALID_8;
    connectedDeviceName = "";
    connectedDeviceNameDirty = true;
    logInfo("USB-Host: MIDI Device Index = %u is unmounted", idx);
  } else {
    logInfo("USB-Host: Unused MIDI Device Index  %u is unmounted", idx);
  }
  DrumIO::led(LedId::MidiConnected, false);
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t num_bytes) {
  if (devIndex == idx) {
    if (num_bytes != 0) {
      uint8_t cable_num;
      uint8_t buffer[48];
      while (1) {
        uint32_t bytes_read = tuh_midi_stream_read(idx, &cable_num, buffer, sizeof(buffer));
        if (bytes_read == 0)
          return;
        if (cable_num == 0) {
          logDebug("Got data");
        }
      }
    }
  }
}

void tuh_midi_tx_cb(uint8_t idx, uint32_t num_bytes) {
  (void)idx;
  (void)num_bytes;
}

static void printConnectedUsbDeviceName(const String& vendorName, const String& productName) {
  logString(Level::Info, "USB-Host MIDI device found: ", LogMode::NoNewline);
  if (vendorName.length() > 0) {
    logString(Level::Info, "Vendor=", LogMode::NoPrefixOrNewline);
    logString(Level::Info, vendorName, LogMode::NoPrefixOrNewline);
  }
  if (productName.length() > 0) {
    logString(Level::Info, " Product=", LogMode::NoPrefixOrNewline);
    logString(Level::Info, productName, LogMode::NoPrefixOrNewline);
  }
  logString(Level::Info, "\n", LogMode::NoPrefixOrNewline);
}

static void updateConnectedDeviceInfo() {
  if (devIndex == TUSB_INDEX_INVALID_8) {
    connectedDeviceName = "";
    webUI.sendUsbHostStatus(connectedDeviceName);
    return;
  }

  tuh_itf_info_t info;
  if (!tuh_midi_itf_get_info(devIndex, &info))
    logError("tuh_midi_itf_get_info failed");

  String vendorName = UsbHost::getVendorName(info);
  String productName = UsbHost::getProductName(info);
  printConnectedUsbDeviceName(vendorName, productName);

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
