#pragma once

// Author attribution: Sourish Raj Gubbala.
#include <stdint.h>

#ifndef DEVICE_DISCONNECTED_C
#define DEVICE_DISCONNECTED_C (-127.0f)
#endif

namespace emberlink {

constexpr int CALIBRATION = 1024;
constexpr int TEMP_CALIB = 3;
constexpr int DEFAULT_SMOKE_THRESHOLD = 150;
constexpr float DEFAULT_TEMP_THRESHOLD = 35.0f;
constexpr uint32_t RTC_MAGIC = 0x454D424C; // "EMBL"
constexpr uint8_t MESH_PACKET_VERSION = 1;
constexpr int RTC_BUFFER_SIZE = 15;

enum class EventType : uint8_t {
    ALERT = 1,
    TEST = 2,
    HEARTBEAT = 3,
    STATUS = 4,
    SILENCE = 5,
    PRE_ALERT = 6,
    PEER_CONFIRM = 7,
    RELAY = 8
};

enum class AlarmLevel : uint8_t {
    LEVEL_1 = 1,
    LEVEL_2 = 2,
    LEVEL_3 = 3
};

struct SensorReading {
    int smoke = 0;
    float temperature = 0.0f;
};

struct RTCBuffer {
    uint32_t magic = RTC_MAGIC;
    uint32_t wakeCount = 0;
    SensorReading readings[RTC_BUFFER_SIZE] = {};
    int baselineSmoke = CALIBRATION;
    uint32_t lastSelfTest = 0;
};

struct MeshAlertPacket {
    uint32_t magic = RTC_MAGIC;
    uint8_t version = MESH_PACKET_VERSION;
    uint8_t reserved0 = 0;
    uint16_t reserved1 = 0;
    uint32_t sequence = 0;
    uint32_t timestampUtc = 0;
    char nodeId[16] = "LANE-01";
    EventType eventType = EventType::ALERT;
    AlarmLevel alarmLevel = AlarmLevel::LEVEL_1;
    int smokeValue = 0;
    float temperatureC = 0.0f;
    int batteryMv = 0;
    uint8_t reserved[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    char originNodeId[16] = "";
    char relayNodeId[16] = "";
    uint8_t hopCount = 0;
    uint8_t maxHops = 0;
    uint8_t confirmationCount = 0;
    uint8_t reserved2[3] = {0, 0, 0};
};

} // namespace emberlink
