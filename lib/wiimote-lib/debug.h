#ifndef WIIMMOTE_DEBUG_H
#define WIIMMOTE_DEBUG_H

#define WIIMOTE_DEBUG

#ifdef WIIMOTE_DEBUG
void logRaw(const char* format, ...);
#define log_printf(...) logRaw(__VA_ARGS__)
#else
#define log_printf(...) {}
#endif

#endif
