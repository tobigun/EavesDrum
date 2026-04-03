// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include "drum_io.h"
#include "log.h"

#include <Arduino.h>
#include <hardware/adc.h>
#ifdef PICO_CYW43_SUPPORTED
#include <cyw43_wrappers.h>
#else
#define __isPicoW false
#endif

#include "touch.h"

#define PIN_LED_0 LED_BUILTIN // built-in LED (green)
#define PIN_LED_1 4 // red
#define PIN_LED_2 5 // white/blue
#if 0
#define PIN_LED_3 9 // yellow
#endif

#define PIN_SWITCH_1 2
#define PIN_SWITCH_2 3

// ADC_BASE_PIN define does not work correctly here, use our own definition
#define ADC_PICO_PICO2_BASE_PIN 26
#define ADC_CHANNEL_COUNT 3

#define SMPS_MODE_PIN 23
#define SMPS_MODE_PWM 1

#define AIRCR_Register (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))

#define WATCHDOG_TIMEOUT_MS 10000

static dma_channel_config adcDmaCfg;
static uint adcDmaChannel;
static uint32_t resetScheduledAtMs = 0;

//#define USE_TOUCH
#ifdef USE_TOUCH
TouchSensor touchSensor(16);
#endif

static void ledInit();
static void buttonInit();
static void adcInit();

void DrumIO::setup(bool usePwmPowerSupply) {
#ifdef WATCHDOG_TIMEOUT_MS
  watchdog_enable(WATCHDOG_TIMEOUT_MS, true);
#endif

  ledInit();
  buttonInit();

#ifdef OVERCLOCK_ADC
  uint32_t adc_clk_freq_hz = clock_get_hz(clk_sys);
  clock_configure(clk_adc, 0, CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, adc_clk_freq_hz, adc_clk_freq_hz);
#endif

  // Driving high the SMPS mode pin (GPIO23), to force the power supply into PWM mode,
  // can greatly reduce the inherent ripple of the SMPS at light load, and therefore the ripple on the ADC supply.
  // This will also increase the power consumption.
  if (!__isPicoW && usePwmPowerSupply) {
    pinMode(SMPS_MODE_PIN, OUTPUT);
    digitalWrite(SMPS_MODE_PIN, SMPS_MODE_PWM);
  }

  adcInit();

#ifdef USE_TOUCH
  touchSensor.init();
#endif
}

static void adcInit() {
  adc_init();

  // https://www.hackster.io/AlexWulff/adc-sampling-and-fft-on-raspberry-pi-pico-f883dd
  adc_fifo_setup(
      true, // Write each completed conversion to the sample FIFO
      true, // Enable DMA data request (DREQ)
      1, // DREQ (and IRQ) asserted when at least 1 sample present
      false, // We won't see the ERR bit because of 8 bit reads; disable.
      true // Shift each sample to 8 bits when pushing to FIFO
  );

  adcDmaChannel = dma_claim_unused_channel(true);
  adcDmaCfg = dma_channel_get_default_config(adcDmaChannel);

  // Reading from constant address, writing to incrementing byte addresses
  channel_config_set_transfer_data_size(&adcDmaCfg, DMA_SIZE_8);
  channel_config_set_read_increment(&adcDmaCfg, false);
  channel_config_set_write_increment(&adcDmaCfg, true);

  // Pace transfers based on availability of ADC samples
  channel_config_set_dreq(&adcDmaCfg, DREQ_ADC);
}

bool DrumIO::initAnalogInPin(pin_size_t analogInPin) {
  bool isValid = (analogInPin >= ADC_PICO_PICO2_BASE_PIN && analogInPin < ADC_PICO_PICO2_BASE_PIN + ADC_CHANNEL_COUNT);
  if (!isValid) {
    return false;
  }
  adc_gpio_init(analogInPin);
  return true;
}

static void readAdcPinInternal(pin_size_t pin, uint8_t* captureBuffer, uint8_t numSamples) {
  adc_select_input(pin - __FIRSTANALOGGPIO);
  adc_fifo_drain();

  dma_channel_configure(adcDmaChannel, &adcDmaCfg,
      captureBuffer, // dst
      &adc_hw->fifo, // src
      numSamples, // transfer count
      true // start immediately
  );

  adc_run(true);
  dma_channel_wait_for_finish_blocking(adcDmaChannel);
  adc_run(false);
}

sensor_value_t DrumIO::readAnalogInPin(pin_size_t pin) {
  uint8_t samples[NUM_SAMPLES];
  readAdcPinInternal(pin, samples, NUM_SAMPLES);

  unsigned int sum = 0;
  for (uint8_t i = 0; i < NUM_SAMPLES; ++i) {
    sum += samples[i];
  }
  return (sum / NUM_SAMPLES) << 2;
}

bool DrumIO::initDigitalOutPin(pin_size_t pin) {
  if (pin < 0 || pin >= __GPIOCNT) {
    return false;
  }
  pinMode(pin, OUTPUT);
  return true;
}

void DrumIO::writeDigitalOutPin(pin_size_t pinNumber, pin_status_t status) {
  digitalWriteFast(pinNumber, status);
}

static void ledTest() {
  DrumIO::led(LedId::HitIndicator, true);
  DrumIO::led(LedId::Network, true);
  DrumIO::led(LedId::MidiConnected, true);
  DrumIO::led(LedId::WatchDog, true);

  delay(200);

  DrumIO::led(LedId::WatchDog, false);
  DrumIO::led(LedId::HitIndicator, false);
  DrumIO::led(LedId::Network, false);
  DrumIO::led(LedId::MidiConnected, false);
}

static void ledInit() {
  pinMode(PIN_LED_0, OUTPUT);
  digitalWrite(PIN_LED_0, HIGH);

  pinMode(PIN_LED_1, OUTPUT);
  digitalWrite(PIN_LED_1, LOW);

  pinMode(PIN_LED_2, OUTPUT);
  digitalWrite(PIN_LED_2, LOW);

#ifdef PIN_LED_3
  pinMode(PIN_LED_3, OUTPUT);
  digitalWrite(PIN_LED_3, LOW);
#endif

  ledTest();
}

static void buttonInit() {
  pinMode(PIN_SWITCH_1, INPUT_PULLUP);
  pinMode(PIN_SWITCH_2, INPUT_PULLUP);
}

void DrumIO::led(LedId id, bool enable) {
  pin_size_t ledPin;
  if (id == LedId::HitIndicator) {
    ledPin = PIN_LED_2;
  } else if (id == LedId::Network) {
    ledPin = PIN_LED_1;
  } else if (id == LedId::MidiConnected) {
    ledPin = PIN_LED_0;
  } else if (id == LedId::WatchDog) {
#ifdef PIN_LED_3
    ledPin = PIN_LED_3;
#else
    return;
#endif
  } else {
    return;
  }

  // avoid blinking the LED on the Pico W, as it is multiplexed by the cyw43.
  // It takes 1ms instead of 1us to toggle the pin with digitalWrite. This would interfer with the timing of the ADC sampling.
  if (__isPicoW && ledPin == LED_BUILTIN) {
    static int lastBuiltinLedEnable = -1;
    if (enable == lastBuiltinLedEnable) {
      return; 
    }
    lastBuiltinLedEnable = enable;
  }

  digitalWrite(ledPin, enable ? HIGH : LOW);
}

bool DrumIO::isButtonPressed(ButtonId id) {
  if (id == ButtonId::Wifi) {
    return digitalRead(PIN_SWITCH_1) == LOW;
  }
  return false;
}

static void blinkLed() {
  static uint32_t last_time = 0;
  static int led_state = HIGH;
  uint32_t cur_time = millis();

  if (cur_time - last_time > 1000) {
    led_state = !led_state;
    DrumIO::led(LedId::WatchDog, led_state);
    last_time = cur_time;
  }
}

void reset() {
  logInfo("Reset\n");
  AIRCR_Register = 0x5FA0004;
}

void DrumIO::update() {
  blinkLed();
#ifdef WATCHDOG_TIMEOUT_MS
  watchdog_update();
#endif

#ifdef USE_TOUCH
  touchSensor.sense();
#endif

  if (resetScheduledAtMs != 0 && millis() >= resetScheduledAtMs) {
    reset();
  }
}

bool DrumIO::requestReset(uint32_t delayMs) {
  if (delayMs == 0) {
    reset();
  } else {
    logInfo("Reset initiated in %u ms\n", delayMs);
    resetScheduledAtMs = millis() + delayMs;
  }
  return true;
}

uint32_t DrumIO::getCpuFrequency() {
  return rp2040.f_cpu();
}

void DrumIO::getMemoryStats(uint32_t& total, uint32_t& free) {
  total = rp2040.getTotalHeap();
  free = rp2040.getFreeHeap();
}
