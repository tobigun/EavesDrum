#pragma once

#include <Arduino.h>

class HardwareSerial {
};

class SerialDummy : public HardwareSerial {
public:
  void begin(int baud) {}

  void print(const char* str) { ::printf("%s", str); }
  void print(arduino::String str) { ::printf("%s", str.c_str()); }
  void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    fflush(stdout);
  }
  void printf_P(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    fflush(stdout);
  }
  void println(const char* str) { ::printf("%s\n", str); fflush(stdout); }
  void println(arduino::String str) { ::printf("%s\n", str.c_str()); fflush(stdout); }
  void println(long n) { ::printf("%ld\n", n); fflush(stdout); }
  void println(unsigned long n) { ::printf("%lu\n", n); fflush(stdout); }
  void println(long long n) { ::printf("%lld\n", n); fflush(stdout); }
  void println(unsigned long long n) { ::printf("%llu\n", n); fflush(stdout); }
  void println() { ::printf("\n"); fflush(stdout); }
  size_t write(uint8_t c) { ::printf("%c", c); return 1; fflush(stdout); }
  size_t write(const uint8_t* s, size_t n) { return ::write(STDOUT_FILENO, s, n); }
};

extern SerialDummy Serial;
