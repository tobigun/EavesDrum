// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef ARDUINO_ARCH_RP2040

#include "drum_io.h"
#include "log.h"

#include <Arduino.h>
#include <hardware/adc.h>

#define GPIO_LED_0 LED_BUILTIN // Use the built-in LED pin
#define GPIO_LED_1 4
#define GPIO_LED_2 5


// ADC_BASE_PIN define does not work correctly here, use our own definition
#define ADC_PICO_PICO2_BASE_PIN 26
#define ADC_CHANNEL_COUNT 3

#define SMPS_MODE_PIN 23
#define SMPS_MODE_PWM 1

#define AIRCR_Register (*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)))

static dma_channel_config cfg;
static uint dma_chan;

static void led_init();


void DrumIO::setup(bool usePwmPowerSupply) {
  led_init();

#ifdef OVERCLOCK_ADC
  uint32_t adc_clk_freq_hz = clock_get_hz(clk_sys);
  clock_configure(clk_adc, 0, CLOCKS_CLK_ADC_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, adc_clk_freq_hz, adc_clk_freq_hz);
#endif

  // Driving high the SMPS mode pin (GPIO23), to force the power supply into PWM mode,
  // can greatly reduce the inherent ripple of the SMPS at light load, and therefore the ripple on the ADC supply.
  // This will also increase the power consumption.
  if (usePwmPowerSupply) {
    pinMode(SMPS_MODE_PIN, OUTPUT);
    digitalWrite(SMPS_MODE_PIN, SMPS_MODE_PWM);
  }

  adc_init();

  // https://www.hackster.io/AlexWulff/adc-sampling-and-fft-on-raspberry-pi-pico-f883dd
  adc_fifo_setup(
      true, // Write each completed conversion to the sample FIFO
      true, // Enable DMA data request (DREQ)
      1, // DREQ (and IRQ) asserted when at least 1 sample present
      false, // We won't see the ERR bit because of 8 bit reads; disable.
      true // Shift each sample to 8 bits when pushing to FIFO
  );

  dma_chan = dma_claim_unused_channel(true);
  cfg = dma_channel_get_default_config(dma_chan);

  // Reading from constant address, writing to incrementing byte addresses
  channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
  channel_config_set_read_increment(&cfg, false);
  channel_config_set_write_increment(&cfg, true);

  // Pace transfers based on availability of ADC samples
  channel_config_set_dreq(&cfg, DREQ_ADC);
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

  dma_channel_configure(dma_chan, &cfg,
      captureBuffer, // dst
      &adc_hw->fifo, // src
      numSamples, // transfer count
      true // start immediately
  );

  adc_run(true);
  dma_channel_wait_for_finish_blocking(dma_chan);
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

static void led_init() {
  pinMode(GPIO_LED_0, OUTPUT);
  digitalWrite(GPIO_LED_0, HIGH);

  pinMode(GPIO_LED_1, OUTPUT);
  digitalWrite(GPIO_LED_1, LOW);

  pinMode(GPIO_LED_2, OUTPUT);
  digitalWrite(GPIO_LED_2, LOW);
}

void DrumIO::led(uint8_t id, bool enable) {
  bool enabled_by_high_level = (id != 0);

  pin_size_t ledPin = GPIO_LED_0;
  if (id == 1) {
    ledPin = GPIO_LED_1;
  } else if (id == 2) {
    ledPin = GPIO_LED_2;
  }

  digitalWrite(ledPin, (enable == enabled_by_high_level) ? HIGH : LOW);
}

void DrumIO::reset() {
  SerialDebug.println("Reset");
  AIRCR_Register = 0x5FA0004;
}

#endif
