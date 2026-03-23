#ifndef DEBUG_H_
#define DEBUG_H_

#include <Arduino.h>

#define panic() panic("AsyncTCP panic at %s:%d\n", __FILE__, __LINE__)

#endif
