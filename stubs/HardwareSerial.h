#pragma once

// Author attribution: Sourish Raj Gubbala.
#include <stdint.h>

class HardwareSerialClass {
public:
    void begin(uint32_t, uint32_t = 0, int = 0, int = 0) {}
    int available() { return 0; }
    int read() { return 0; }
    void print(const char*) {}
    void print(const String&) {}
    void println(const char*) {}
    void write(uint8_t) {}
    void flush() {}
};

extern HardwareSerialClass Serial2;
