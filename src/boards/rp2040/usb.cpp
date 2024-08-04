// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#include <Arduino.h>
#include <tusb.h>

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
    tud_task();
    mutex_exit(&__usb_mutex);
  }
}

static int64_t timer_task(__unused alarm_id_t id, __unused void* user_data)
{
  irq_set_pending(__usb_task_irq);
  return USB_TASK_INTERVAL;
}

void setupUsb()
{
  mutex_init(&__usb_mutex);

  tusb_init();

  __usb_task_irq = user_irq_claim_unused(true);
  irq_set_exclusive_handler(__usb_task_irq, usb_irq);
  irq_set_enabled(__usb_task_irq, true);

  add_alarm_in_us(USB_TASK_INTERVAL, timer_task, nullptr, true);
}
