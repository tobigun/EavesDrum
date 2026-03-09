#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_PRINTF(...)
//#define DEBUG_PRINTF(...) debug_printf(__VA_ARGS__)

void debug_printf(const char *format, ...);

#ifdef __cplusplus
}
#endif
