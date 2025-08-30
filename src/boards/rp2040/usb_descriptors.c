/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
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
 *
 */

#include "tusb.h"

/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]       NET | VENDOR | MIDI | HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) | _PID_MAP(ECM_RNDIS, 5) | _PID_MAP(NCM, 5))

// String Descriptor Index
enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
  STRID_INTERFACE_NET,
#if CFG_TUD_CDC
  STRID_INTERFACE_SERIAL,
#endif
#if CFG_TUD_MSC
  STRID_INTERFACE_MSC,
#endif
#if CFG_TUD_MIDI
  STRID_INTERFACE_MIDI,
#endif
  STRID_MAC
};

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {
  .bLength = sizeof(tusb_desc_device_t),
  .bDescriptorType = TUSB_DESC_DEVICE,
  .bcdUSB = 0x0200,

  // Use Interface Association Descriptor (IAD) device class
  .bDeviceClass = TUSB_CLASS_MISC,
  .bDeviceSubClass = MISC_SUBCLASS_COMMON,
  .bDeviceProtocol = MISC_PROTOCOL_IAD,

  .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

  .idVendor = 0xCafe,
  .idProduct = USB_PID,
  .bcdDevice = 0x0101,

  .iManufacturer = STRID_MANUFACTURER,
  .iProduct = STRID_PRODUCT,
  .iSerialNumber = STRID_SERIAL,

  .bNumConfigurations = 1 // multiple configurations
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const* tud_descriptor_device_cb(void)
{
  return (uint8_t const*)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

#if CFG_TUD_MIDI
#define USBD_MIDI_EPOUT 0x05
#define USBD_MIDI_EPIN 0x86
#define USBD_MIDI_EPSIZE 64
#endif

#define USBD_NET_EPNOTIF 0x81
#define USBD_NET_EPOUT 0x02
#define USBD_NET_EPIN 0x82

#if CFG_TUD_MSC
#define USBD_MSC_EPOUT 0x03
#define USBD_MSC_EPIN 0x83
#define USBD_MSC_EPSIZE 64
#endif


#if CFG_TUD_CDC
#define USBD_CDC_EP_CMD (0x84)
#define USBD_CDC_EP_OUT (0x04)
#define USBD_CDC_EP_IN (0x85)
#define USBD_CDC_CMD_MAX_SIZE (8)
#define USBD_CDC_IN_OUT_MAX_SIZE (64)
#endif

enum {
  ITF_NUM_NET = 0,
  ITF_NUM_NET_DATA,
#if CFG_TUD_CDC
  ITF_NUM_SERIAL,
  ITF_NUM_SERIAL_DATA,
#endif
#if CFG_TUD_MSC
  ITF_NUM_MSC,
#endif
#if CFG_TUD_MIDI
  ITF_NUM_MIDI, // Audio-Control (AC)
  ITF_NUM_MIDI_STREAM, // Midi Stream
#endif
  ITF_NUM_TOTAL
};

#if CFG_TUD_MSC
#define MSC_DESC_LEN TUD_MSC_DESC_LEN
#else
#define MSC_DESC_LEN 0
#endif

#if CFG_TUD_CDC
#define SERIAL_DESC_LEN TUD_CDC_DESC_LEN
#else
#define SERIAL_DESC_LEN 0
#endif

#if CFG_TUD_MIDI
#define MIDI_DESC_LEN TUD_MIDI_DESC_LEN
#else
#define MIDI_DESC_LEN 0
#endif

#define CONFIG_COMMON_LEN (TUD_CONFIG_DESC_LEN + SERIAL_DESC_LEN + MSC_DESC_LEN + MIDI_DESC_LEN)

#if CFG_TUD_ECM_RNDIS
#define CONFIG_TOTAL_LEN (CONFIG_COMMON_LEN + TUD_RNDIS_DESC_LEN)
#else
#define CONFIG_TOTAL_LEN (CONFIG_COMMON_LEN + TUD_CDC_NCM_DESC_LEN)
#endif

static uint8_t const configuration[] = {
  // Config number (index+1), interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),
#if CFG_TUD_ECM_RNDIS
  TUD_RNDIS_DESCRIPTOR(ITF_NUM_NET, STRID_INTERFACE_NET, USBD_NET_EPNOTIF, 8, USBD_NET_EPOUT, USBD_NET_EPIN, CFG_TUD_NET_ENDPOINT_SIZE),
#else
  TUD_CDC_NCM_DESCRIPTOR(ITF_NUM_NET, STRID_INTERFACE_NET, STRID_MAC, USBD_NET_EPNOTIF, 64, USBD_NET_EPOUT, USBD_NET_EPIN, CFG_TUD_NET_ENDPOINT_SIZE, CFG_TUD_NET_MTU),
#endif
#if CFG_TUD_CDC
  TUD_CDC_DESCRIPTOR(ITF_NUM_SERIAL, STRID_INTERFACE_SERIAL, USBD_CDC_EP_CMD, USBD_CDC_CMD_MAX_SIZE, USBD_CDC_EP_OUT, USBD_CDC_EP_IN, USBD_CDC_IN_OUT_MAX_SIZE),
#endif
#if CFG_TUD_MSC
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, STRID_INTERFACE_MSC, USBD_MSC_EPOUT, USBD_MSC_EPIN, USBD_MSC_EPSIZE),
#endif
#if CFG_TUD_MIDI
  TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, STRID_INTERFACE_MIDI, USBD_MIDI_EPOUT, USBD_MIDI_EPIN, USBD_MIDI_EPSIZE)
#endif
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const* tud_descriptor_configuration_cb(uint8_t index)
{
  return configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

#define PROJECT_NAME "EavesDrum"

// array of pointer to string descriptors
static char const* string_desc_arr[] = {
  [STRID_LANGID] = (const char[]) { 0x09, 0x04 }, // supported language is English (0x0409)
  [STRID_MANUFACTURER] = PROJECT_NAME,
  [STRID_PRODUCT] = PROJECT_NAME,
  [STRID_SERIAL] = "123456",
  [STRID_INTERFACE_NET] = "USB Network",
#if CFG_TUD_CDC  
  [STRID_INTERFACE_SERIAL] = PROJECT_NAME " Serial",
#endif
#if CFG_TUD_MSC
  [STRID_INTERFACE_MSC] = PROJECT_NAME " Drive",
#endif
#if CFG_TUD_MIDI
  [STRID_INTERFACE_MIDI] = PROJECT_NAME " MIDI",
#endif

  // STRID_MAC index is handled separately
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void)langid;

  unsigned int chr_count = 0;

  if (STRID_LANGID == index) {
    memcpy(&_desc_str[1], string_desc_arr[STRID_LANGID], 2);
    chr_count = 1;
  } else if (STRID_MAC == index) {
    // Convert MAC address into UTF-16

    for (unsigned i = 0; i < sizeof(tud_network_mac_address); i++) {
      _desc_str[1 + chr_count++] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 4) & 0xf];
      _desc_str[1 + chr_count++] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 0) & 0xf];
    }
  } else {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
      return NULL;

    const char* str = string_desc_arr[index];

    // Cap at max char
    chr_count = (uint8_t)strlen(str);
    if (chr_count > (TU_ARRAY_SIZE(_desc_str) - 1))
      chr_count = TU_ARRAY_SIZE(_desc_str) - 1;

    // Convert ASCII string into UTF-16
    for (unsigned int i = 0; i < chr_count; i++) {
      _desc_str[1 + i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

  return _desc_str;
}
