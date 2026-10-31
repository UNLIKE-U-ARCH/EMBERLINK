#pragma once

// Author attribution: Sourish Raj Gubbala.
#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include "../stubs/Arduino.h"
#endif

namespace emberlink {
namespace subnode {

int readSmokeAverage(int analogPin, int sampleCount = 5);
float readTemperatureC(int sensorPin);
bool isSensorAlarm(int smokeValue,
                   float temperatureC,
                   int smokeThreshold,
                   float temperatureThreshold,
                   int alarmLevel);

} // namespace subnode
} // namespace emberlink
