// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include "soc/soc_caps.h"

#define USB_VID 0x303A
#define USB_PID 0x1001
#define USB_MANUFACTURER "Waveshare"
#define USB_PRODUCT "Waveshare ESP32 S3 Pico"
#define USB_SERIAL ""

static const uint8_t TX = 43;
static const uint8_t RX = 44;

static const uint8_t SDA = 8;
static const uint8_t SCL = 9;

static const uint8_t SS    = 5;
static const uint8_t MOSI  = 35;
static const uint8_t MISO  = 37;
static const uint8_t SDO  = 35;
static const uint8_t SDI  = 37;
static const uint8_t SCK   = 36;

static const uint8_t A0 = 1;
static const uint8_t A1 = 2;
static const uint8_t A2 = 3;
static const uint8_t A3 = 4;
static const uint8_t A4 = 5;
static const uint8_t A5 = 6;
static const uint8_t A6 = 7;
static const uint8_t A7 = 8;
static const uint8_t A8 = 9;
static const uint8_t A9 = 10;
static const uint8_t A10 = 11;
static const uint8_t A11 = 12;
static const uint8_t A12 = 13;

static const uint8_t T1 = 1;
static const uint8_t T3 = 3;
static const uint8_t T5 = 5;
static const uint8_t T6 = 6;
static const uint8_t T7 = 7;
static const uint8_t T8 = 8;
static const uint8_t T9 = 9;
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;
static const uint8_t T14 = 14;

//static const uint8_t VBAT_SENSE = 2;
//static const uint8_t VBUS_SENSE = 34;

// User LED 
#define LED_BUILTIN 21
#define BUILTIN_LED  LED_BUILTIN // backward compatibility

static const uint8_t RGB_DATA = 21;
// RGB_BUILTIN and RGB_BRIGHTNESS can be used in new Arduino API neopixelWrite()
#define RGB_BUILTIN (RGB_DATA + SOC_GPIO_PIN_COUNT)  
#define RGB_BRIGHTNESS 64

#endif /* Pins_Arduino_h */
