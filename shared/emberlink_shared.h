#pragma once

// Author attribution: Sourish Raj Gubbala.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "emberlink_types.h"

namespace emberlink {

inline bool isValidTemperature(float value) {
    return !isnan(value) && value != DEVICE_DISCONNECTED_C && value > -100.0f;
}

inline int normalizeSmokeReading(int rawSample) {
    if (rawSample < 0) {
        return 0;
    }
    if (rawSample > 4095) {
        return 4095;
    }
    return rawSample;
}

inline int computeAverage(const int values[], size_t count) {
    if (count == 0) {
        return 0;
    }

    long sum = 0;
    for (size_t i = 0; i < count; ++i) {
        sum += values[i];
    }
    return static_cast<int>(sum / static_cast<long>(count));
}

inline bool isAlarmCondition(int smokeValue,
                             float temperatureC,
                             int smokeThreshold,
                             float tempThreshold,
                             AlarmLevel level) {
    const bool smokeDetected = smokeValue >= smokeThreshold;
    const bool heatDetected = isValidTemperature(temperatureC) && (temperatureC >= tempThreshold);

    switch (level) {
        case AlarmLevel::LEVEL_2:
            return smokeDetected;
        case AlarmLevel::LEVEL_3:
            return smokeDetected && heatDetected;
        case AlarmLevel::LEVEL_1:
        default:
            return smokeDetected || heatDetected;
    }
}

inline void seedRtcBuffer(RTCBuffer& rtc, int baselineSmoke) {
    rtc.magic = RTC_MAGIC;
    rtc.wakeCount = 0;
    rtc.baselineSmoke = baselineSmoke;
    rtc.lastSelfTest = 0;
    for (int i = 0; i < RTC_BUFFER_SIZE; ++i) {
        rtc.readings[i].smoke = 0;
        rtc.readings[i].temperature = NAN;
    }
}

inline void appendReading(RTCBuffer& rtc, int smokeValue, float temperatureC) {
    const int index = (rtc.wakeCount >= RTC_BUFFER_SIZE) ? (RTC_BUFFER_SIZE - 1) : rtc.wakeCount;
    rtc.readings[index].smoke = smokeValue;
    rtc.readings[index].temperature = temperatureC;
    rtc.wakeCount = (rtc.wakeCount + 1) % RTC_BUFFER_SIZE;
}

inline bool isValidPacket(const MeshAlertPacket& packet) {
    if (packet.magic != RTC_MAGIC) {
        return false;
    }
    if (packet.version != MESH_PACKET_VERSION) {
        return false;
    }
    if (packet.nodeId[0] == '\0') {
        return false;
    }
    if (packet.eventType < EventType::ALERT || packet.eventType > EventType::SILENCE) {
        return false;
    }
    if (packet.alarmLevel < AlarmLevel::LEVEL_1 || packet.alarmLevel > AlarmLevel::LEVEL_3) {
        return false;
    }
    return true;
}

inline void copyPacketToBuffer(const MeshAlertPacket& packet, uint8_t* out, size_t outLength) {
    if (out == nullptr || outLength < sizeof(MeshAlertPacket)) {
        return;
    }
    memcpy(out, &packet, sizeof(MeshAlertPacket));
}

inline void copyBufferToPacket(const uint8_t* in, size_t inLength, MeshAlertPacket& packet) {
    if (in == nullptr || inLength < sizeof(MeshAlertPacket)) {
        return;
    }
    memcpy(&packet, in, sizeof(MeshAlertPacket));
}

} // namespace emberlink
