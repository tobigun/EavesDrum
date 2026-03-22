// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <tusb.h>
#include "usb_device.h"

// Note: using is the way the Arduino framework triggers the tud_task method.
// As the timer just triggers an interrupt that is executed on core 0 (the main core),
// we can also just call tud_task directly from the main loop.
// This should be ok for the tud_task() 1ms poll requirement. The main loop should never be blocked for more than 1ms,
// as otherwise the trigger handling would not work correctly anymore.
#undef USB_TASK_TRIGGERED_BY_TIMER

#ifdef USB_TASK_TRIGGERED_BY_TIMER
// Big, global USB mutex, shared with all USB devices to make sure we don't
// have multiple cores updating the TUSB state in parallel
mutex_t __usb_mutex;

// USB processing will be a periodic timer task
#define USB_TASK_INTERVAL 1000
static int __usb_task_irq;

int usb_hid_poll_interval __attribute__((weak)) = 10;

static void usb_irq()
{
  // if the mutex is already owned, then we are in user code
  // in this file which will do a tud_task itself, so we'll just do nothing
  // until the next tick; we won't starve
  if (mutex_try_enter(&__usb_mutex, nullptr)) {
    tud_task_ext(UINT32_MAX, true);
    mutex_exit(&__usb_mutex);
  }
}

static int64_t timer_task(__unused alarm_id_t id, __unused void* user_data)
{
  irq_set_pending(__usb_task_irq);
  return USB_TASK_INTERVAL;
}
#endif

void UsbDevice::begin()
{
  tusb_init();

#if USB_TASK_TRIGGERED_BY_TIMER
  mutex_init(&__usb_mutex);

  __usb_task_irq = user_irq_claim_unused(true);
  irq_set_exclusive_handler(__usb_task_irq, usb_irq);
  irq_set_enabled(__usb_task_irq, true);

  add_alarm_in_us(USB_TASK_INTERVAL, timer_task, nullptr, true);
#endif
}

void UsbDevice::update() {
#ifndef USB_TASK_TRIGGERED_BY_TIMER
  tud_task_ext(0, false);
#endif
}
