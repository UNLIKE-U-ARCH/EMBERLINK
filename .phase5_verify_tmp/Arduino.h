#pragma once
#include <stdint.h>
#define DEVICE_DISCONNECTED_C (-127.0f)
class SerialClass {
public:
    void println(const char*) {}
    void print(const char*) {}
    void printf(const char*, ...) {}
};
inline SerialClass Serial;
inline int analogRead(int) { return 1024; }
inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}
inline void pinMode(int, int) {}
