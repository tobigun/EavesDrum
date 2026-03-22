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

#include "midi_transport_usb_host.h"

#ifdef ENABLE_MIDI_USB_HOST_TRANSPORT

#include <tusb.h>
#include "log.h"
#include "pio_usb.h"
#include "drum_io.h"
#include "webui.h"

static uint8_t dev_idx = TUSB_INDEX_INVALID_8;

static bool connectedDeviceNameDirty = false;
static String connectedDeviceName;

void updateConnectedDeviceInfo();

int getFreeDmaChannelForPioUsb() {
  // pio usb wants a fixed dma channel. Lower channels are unlikely free so query for an unused channel
  int usbDmaTx = dma_claim_unused_channel(true);
  dma_channel_unclaim(usbDmaTx);
  return usbDmaTx;
}

void MidiTransport_UsbHost::begin() {
  DrumIO::led(LedId::MidiConnected, false);

  int usbDmaTx = getFreeDmaChannelForPioUsb();

  pio_usb_configuration_t pio_cfg =   {
    PIO_USB_DP_PIN_DEFAULT,
    PIO_USB_TX_DEFAULT,
    PIO_SM_USB_TX_DEFAULT,
    (uint8_t) usbDmaTx,
    PIO_USB_RX_DEFAULT,
    PIO_SM_USB_RX_DEFAULT,
    PIO_SM_USB_EOP_DEFAULT,
    NULL,
    PIO_USB_DEBUG_PIN_NONE,
    PIO_USB_DEBUG_PIN_NONE,
    false,
    PIO_USB_PINOUT_DPDM
  };

  tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);  
  tuh_init(BOARD_TUH_RHPORT);
}

void MidiTransport_UsbHost::update() {
  tuh_task_ext(0, false);
  if (connectedDeviceNameDirty) {
    updateConnectedDeviceInfo();
    connectedDeviceNameDirty = false;
  }
}

void MidiTransport_UsbHost::shutdown() {
  tuh_deinit(BOARD_TUH_RHPORT);
}

String MidiTransport_UsbHost::getConnectedDeviceName() {
  return connectedDeviceName;
}

uint8_t MidiSerialUsbHost::getDeviceIndex() {
  return dev_idx;
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

  if (dev_idx == TUSB_INDEX_INVALID_8) {
    // then no MIDI device is currently connected
    dev_idx = idx;
    connectedDeviceNameDirty = true;
  } else {
    logError("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled");
  }
  DrumIO::led(LedId::MidiConnected, true);
}

// Invoked when device with MIDI interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx) {
  if (idx == dev_idx) {
    dev_idx = TUSB_INDEX_INVALID_8;
    connectedDeviceName = "";
    connectedDeviceNameDirty = true;
    logInfo("USB-Host: MIDI Device Index = %u is unmounted", idx);
  } else {
    logInfo("USB-Host: Unused MIDI Device Index  %u is unmounted", idx);
  }
  DrumIO::led(LedId::MidiConnected, false);
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t num_bytes) {
  if (dev_idx == idx) {
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

static String descriptorToString(uint16_t* buffer) {
  String value;
  uint8_t len = ((*buffer) & 0xFF) / 2;
  for (uint8_t idx = 1; idx < len; idx++) {
    value += String((char)buffer[idx]);
  }
  return value;
}

void updateConnectedDeviceInfo() {
  if (dev_idx == TUSB_INDEX_INVALID_8) {
    connectedDeviceName = "";
    webUI.sendUsbHostStatus(connectedDeviceName);
    return;
  }

  uint16_t buffer[128];
  tuh_itf_info_t info;
  if (!tuh_midi_itf_get_info(dev_idx, &info))
    logError("tuh_midi_itf_get_info failed");

  String vendorName;
  String productName;
  String serialName;

  if (tuh_descriptor_get_string_langid_sync(info.daddr, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
    logString(Level::Info, "USB-Host device found: ", LogMode::NoNewline);

    uint16_t langid = buffer[1];
    if (tuh_descriptor_get_manufacturer_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      vendorName = descriptorToString(buffer);
      logString(Level::Info, "Vendor=", LogMode::NoPrefixOrNewline);
      logString(Level::Info, vendorName, LogMode::NoPrefixOrNewline);
    }
    if (tuh_descriptor_get_product_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      productName = descriptorToString(buffer);
      logString(Level::Info, " Product=", LogMode::NoPrefixOrNewline);
      logString(Level::Info, productName, LogMode::NoPrefixOrNewline);
    }
    if (tuh_descriptor_get_serial_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      serialName = descriptorToString(buffer);
      logString(Level::Info, " Serial=", LogMode::NoPrefixOrNewline);
      logString(Level::Info, serialName, LogMode::NoPrefixOrNewline);
    }
    logString(Level::Info, "\n", LogMode::NoPrefixOrNewline);
  }

  if (productName.length() > 0 && vendorName.length() > 0) {
    connectedDeviceName = vendorName + " " + productName;
  } else if (productName.length() > 0) {
    connectedDeviceName = productName;
  } else if (vendorName.length() > 0) {
    connectedDeviceName = vendorName;
  } else {
    connectedDeviceName = serialName;
  }

  webUI.sendUsbHostStatus(connectedDeviceName);
}

#endif
