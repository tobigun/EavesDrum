#include "midi_serial_usb_host.h"

#ifdef ENABLE_MIDI_USB_HOST_TRANSPORT

#include <Arduino.h>

MidiSerialUsbHost usbh_midi;

bool update_strings = false;

static void print_string_descriptor(uint16_t* buffer) {
  uint8_t len = ((*buffer) & 0xFF) / 2;
  for (uint8_t idx = 1; idx < len; idx++) {
    Serial1.printf("%c", buffer[idx]);
  }
  Serial1.printf("\r\n");
}

void MidiSerialUsbHost::printConnectedDevice() {
  uint16_t buffer[128];
  update_strings = false;
  tuh_itf_info_t info;
  if (!tuh_midi_itf_get_info(dev_idx, &info))
    Serial1.printf("tuh_midi_itf_get_info failed\r\n");
  if (tuh_descriptor_get_string_langid_sync(info.daddr, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
    uint16_t langid = buffer[1];
    if (tuh_descriptor_get_manufacturer_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      Serial1.printf("manufacturer: ");
      print_string_descriptor(buffer);
    }
    if (tuh_descriptor_get_product_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      Serial1.printf("product: ");
      print_string_descriptor(buffer);
    }
    if (tuh_descriptor_get_serial_string_sync(info.daddr, langid, buffer, sizeof(buffer)) == XFER_RESULT_SUCCESS) {
      Serial1.printf("serial: ");
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
  Serial1.printf("MIDI Device Index = %u, MIDI device address = %u, %u IN cables, OUT %u cables\r\n", idx,
      mount_cb_data->daddr, mount_cb_data->rx_cable_count, mount_cb_data->tx_cable_count);

  if (usbh_midi.getDeviceIndex() == TUSB_INDEX_INVALID_8) {
    // then no MIDI device is currently connected
    usbh_midi.setDeviceIndex(idx);
  } else {
    Serial1.printf("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
  }
  update_strings = true;
}

// Invoked when device with MIDI interface is un-mounted
void tuh_midi_umount_cb(uint8_t idx) {
  if (idx == usbh_midi.getDeviceIndex()) {
    usbh_midi.setDeviceIndex(TUSB_INDEX_INVALID_8);
    Serial1.printf("MIDI Device Index = %u is unmounted\r\n", idx);
  } else {
    Serial1.printf("Unused MIDI Device Index  %u is unmounted\r\n", idx);
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
          Serial1.println("Got data");
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
