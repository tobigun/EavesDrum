# ring\_buffer\_lib
Ring buffer library targeted at the Raspberry Pi Pico SDK; IRQ and multi-core safe

Projects that use this library must create a file called `ring_buffer_lib_config.h`
that contains the following code.

```
// You can define a larger ring buffer size type if you want
// RING_BUFFER_SIZE_TYPE has to be able to hold a value less
// than or equal to the maximum number of bytes in a ring buffer
// Uncomment and modify the next line if the default (uint8_t)
// is not big enough
// #define RING_BUFFER_SIZE_TYPE uint8_t

// If you don't need to support multiple cores modifying the
// ring buffer, set RING_BUFFER_MULTICORE_SUPPORT to 0; it
// will save time and space
// Otherwise set it to 1. Uncomment the next line to enable
// multicore support
// #define RING_BUFFER_MULTICORE_SUPPORT 1

// If RING_BUFFER_MULTICORE_SUPPORT is 0, then safe version
// of ring buffer calls by default disable all IRQs on the
// current core, manipulate the buffer, and then restore IRQs.
// That is probably overkill for most applications. For
// example, if you are using this ring buffer with a peripheral
// driver, you can store the IRQ number to disable in the
// critical_section_data field of the ring_buffer structure
// in the ring_buffer_init() function call and then use
// that value to enable or disable only that peripheral IRQ.
// Uncomment the example below to do that
//#include "hardware/irq.h"
//
//#define RING_BUFFER_ENTER_CRITICAL(X) \
//    do {irq_set_enabled(ring_buf->critical_section_data, false);} while (0)
//
//#define RING_BUFFER_EXIT_CRITICAL(X) \
//    do {irq_set_enabled(ring_buf->critical_section_data, true);} while (0)```
```

See the [midi\_uart\_lib](https://github.com/rppicomidi/midi_uart_lib) project
for a sample `ring_buffer_lib_config.h` file and for an example of how the library
is used.
