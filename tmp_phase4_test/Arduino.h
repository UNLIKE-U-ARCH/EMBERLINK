#pragma once
#include <stdint.h>
#include <math.h>
#define A0 0
#define DEVICE_DISCONNECTED_C (-127.0f)
using ulong = unsigned long;
inline int analogRead(int) { return 0; }
inline ulong millis() { return 0; }
inline void delay(unsigned long) {}
