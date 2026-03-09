#include <Arduino.h>
#include "ble_midi_debug.h"

void debug_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial1.print(buffer);
    
    va_end(args);
}
