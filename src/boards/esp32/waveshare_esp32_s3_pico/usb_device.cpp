#include <USB.h>
#include <USBMIDI.h>
#include <esp32-hal-tinyusb.h>
#include "usb_device.h"

extern "C" uint16_t tusb_drum_midi_load_descriptor(uint8_t *dst, uint8_t *itf) {
  uint8_t str_index = tinyusb_add_string_descriptor("EavesDrum MIDI");
  uint8_t ep_num = tinyusb_get_free_duplex_endpoint();
  TU_VERIFY(ep_num != 0);
  uint8_t descriptor[TUD_MIDI_DESC_LEN] = {
      // Interface number, string index, EP Out & EP In address, EP size
      TUD_MIDI_DESCRIPTOR(*itf, str_index, ep_num, (uint8_t)(0x80 | ep_num),
                          64)};
  *itf += 1;
  memcpy(dst, descriptor, TUD_MIDI_DESC_LEN);
  return TUD_MIDI_DESC_LEN;
}

void UsbDevice::begin() {
  tinyusb_enable_interface(USB_INTERFACE_MIDI, TUD_MIDI_DESC_LEN, tusb_drum_midi_load_descriptor);
}
