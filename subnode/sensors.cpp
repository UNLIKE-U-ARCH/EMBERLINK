// Author attribution: Sourish Raj Gubbala.
#include "sensors.h"

#include "../shared/emberlink_shared.h"

namespace emberlink {
namespace subnode {

namespace {
int clampSmokeThreshold(int threshold) {
    if (threshold <= 0) {
        return DEFAULT_SMOKE_THRESHOLD;
    }
    return threshold;
}

float clampTemperatureThreshold(float threshold) {
    if (!isfinite(threshold) || threshold <= 0.0f) {
        return DEFAULT_TEMP_THRESHOLD;
    }
    return threshold;
}
}

int readSmokeAverage(int analogPin, int sampleCount) {
    if (sampleCount <= 0 || analogPin < 0) {
        return CALIBRATION;
    }

    int sum = 0;
    for (int i = 0; i < sampleCount; ++i) {
        const int raw = analogRead(analogPin);
        if (raw < 0 || raw > 4095) {
            continue;
        }
        sum += raw;
        delay(20);
    }

    if (sampleCount <= 0) {
        return CALIBRATION;
    }

    const int average = sum / sampleCount;
    return average > 0 ? average : CALIBRATION;
}

float readTemperatureC(int sensorPin) {
    if (sensorPin < 0) {
        return DEVICE_DISCONNECTED_C;
    }

#ifndef EMBERLINK_TEMP_SENSOR_ENABLED
#define EMBERLINK_TEMP_SENSOR_ENABLED 0
#endif

    if (EMBERLINK_TEMP_SENSOR_ENABLED == 0) {
        return DEVICE_DISCONNECTED_C;
    }

    return DEVICE_DISCONNECTED_C;
}

bool isSensorAlarm(int smokeValue,
                   float temperatureC,
                   int smokeThreshold,
                   float temperatureThreshold,
                   int alarmLevel) {
    const int clampedLevel = (alarmLevel < static_cast<int>(AlarmLevel::LEVEL_1)) ?
        static_cast<int>(AlarmLevel::LEVEL_1) :
        ((alarmLevel > static_cast<int>(AlarmLevel::LEVEL_3)) ?
            static_cast<int>(AlarmLevel::LEVEL_3) : alarmLevel);

    const int effectiveSmokeThreshold = clampSmokeThreshold(smokeThreshold);
    const float effectiveTempThreshold = clampTemperatureThreshold(temperatureThreshold);

    const bool smokeDetected = smokeValue >= effectiveSmokeThreshold;
    const bool heatDetected = isValidTemperature(temperatureC) && (temperatureC >= effectiveTempThreshold);

    switch (clampedLevel) {
        case static_cast<int>(AlarmLevel::LEVEL_2):
            return smokeDetected;
        case static_cast<int>(AlarmLevel::LEVEL_3):
            return smokeDetected && heatDetected;
        case static_cast<int>(AlarmLevel::LEVEL_1):
        default:
            return smokeDetected || heatDetected;
    }
}

} // namespace subnode
} // namespace emberlink
