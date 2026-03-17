/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 hathach for Adafruit Industries
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

 #include "midi_serial_usb_host.h"

#ifdef ENABLE_MIDI_USB_HOST_TRANSPORT

#include <Arduino.h>

#define DEBUG_PRINTF Serial1.printf

MidiSerialUsbHost usbh_midi;

bool update_strings = false;

static void print_string_descriptor(uint16_t* buffer) {
  uint8_t len = ((*buffer) & 0xFF) / 2;
  for (uint8_t idx = 1; idx < len; idx++) {
    DEBUG_PRINTF("%c", buffer[idx]);
  }
  DEBUG_PRINTF("\r\n");
}

void MidiSerialUsbHost::printConnectedDevice() {
  uint16_t buffer[128];
  update_strings = false;
  tuh_itf_info_t info;
  if (!tuh_midi_itf_get_info(dev_idx, &info))
    DEBUG_PRINTF("tuh_midi_itf_get_info failed\r\n");
  if (tuh_descriptor_get_string_langid_sync(info.daddr, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
    uint16_t langid = buffer[1];
    if (tuh_descriptor_get_manufacturer_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      DEBUG_PRINTF("manufacturer: ");
      print_string_descriptor(buffer);
    }
    if (tuh_descriptor_get_product_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      DEBUG_PRINTF("product: ");
      print_string_descriptor(buffer);
    }
    if (tuh_descriptor_get_serial_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      DEBUG_PRINTF("serial: ");
      print_string_descriptor(buffer);
    }
  }
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
  DEBUG_PRINTF("MIDI Device Index = %u, MIDI device address = %u, %u IN cables, OUT %u cables\r\n", idx,
      mount_cb_data->daddr, mount_cb_data->rx_cable_count, mount_cb_data->tx_cable_count);

  if (usbh_midi.getDeviceIndex() == TUSB_INDEX_INVALID_8) {
    // then no MIDI device is currently connected
    usbh_midi.setDeviceIndex(idx);
  } else {
    DEBUG_PRINTF("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
  }
  update_strings = true;
}

// Invoked when device with MIDI interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx) {
  if (idx == usbh_midi.getDeviceIndex()) {
    usbh_midi.setDeviceIndex(TUSB_INDEX_INVALID_8);
    DEBUG_PRINTF("MIDI Device Index = %u is unmounted\r\n", idx);
  } else {
    DEBUG_PRINTF("Unused MIDI Device Index  %u is unmounted\r\n", idx);
  }
}

void tuh_midi_rx_cb(uint8_t idx, uint32_t num_bytes) {
  if (usbh_midi.getDeviceIndex() == idx) {
    if (num_bytes != 0) {
      uint8_t cable_num;
      uint8_t buffer[48];
      while (1) {
        uint32_t bytes_read = tuh_midi_stream_read(idx, &cable_num, buffer, sizeof(buffer));
        if (bytes_read == 0)
          return;
        if (cable_num == 0) {
          DEBUG_PRINTF("Got data\r\n");
        }
      }
    }
  }
}

void tuh_midi_tx_cb(uint8_t idx, uint32_t num_bytes) {
  (void)idx;
  (void)num_bytes;
}

#endif
