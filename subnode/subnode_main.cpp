// Author attribution: Sourish Raj Gubbala.
#include <Arduino.h>
#if __has_include(<WiFi.h>)
#include <WiFi.h>
#endif
#if __has_include(<esp_now.h>)
#include <esp_now.h>
#endif

#include "power.h"
#include "sensors.h"
#include "subnode_config.h"
#include "../shared/emberlink_shared.h"

namespace emberlink {
namespace subnode {

namespace {
constexpr uint32_t kDeepSleepSeconds = 60;
constexpr uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
}

RTC_DATA_ATTR RTCBuffer gRtcState = {};
RTC_DATA_ATTR uint32_t gBootCount = 0;
RTC_DATA_ATTR uint32_t gSequence = 0;
RTC_DATA_ATTR uint32_t gLastLocalBuzzerMs = 0;

static Config gConfig;

static bool shouldSoundLocalBuzzer() {
    if (!gConfig.buzzerEnabled || gConfig.buzzerPin < 0 || gConfig.buzzerPulseMs == 0) {
        return false;
    }

    const uint32_t nowMs = millis();
    if (gLastLocalBuzzerMs != 0 && (nowMs - gLastLocalBuzzerMs) < gConfig.buzzerCooldownMs) {
        return false;
    }

    gLastLocalBuzzerMs = nowMs;
    return true;
}

static void initRtcState() {
    if (gRtcState.magic != RTC_MAGIC) {
        seedRtcBuffer(gRtcState, CALIBRATION);
    }
}

static void initEspNow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        return;
    }

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, kBroadcastMac, sizeof(kBroadcastMac));
    peer.channel = 1;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("ESP-NOW peer add failed");
    }
}

static int readBatteryMvSafe() {
    const int raw = analogReadMilliVolts(A0);
    if (raw <= 0 || raw > 5000) {
        return 0;
    }
    return raw;
}

static bool sendEspNowPacket(const uint8_t* buffer, size_t length) {
    if (buffer == nullptr || length == 0) {
        return false;
    }

    for (int attempt = 0; attempt < 3; ++attempt) {
        const esp_err_t result = esp_now_send(kBroadcastMac, const_cast<uint8_t*>(buffer), length);
        if (result == ESP_OK) {
            return true;
        }
        delay(25);
    }

    return false;
}

static bool broadcastAlert(int smokeValue, float temperatureC, EventType eventType) {
    MeshAlertPacket packet;
    strncpy(packet.nodeId, gConfig.nodeId, sizeof(packet.nodeId) - 1);
    packet.sequence = ++gSequence;
    packet.timestampUtc = millis() / 1000UL;
    packet.eventType = eventType;
    packet.alarmLevel = static_cast<AlarmLevel>(gConfig.alarmLevel);
    packet.smokeValue = smokeValue;
    packet.temperatureC = temperatureC;
    packet.batteryMv = readBatteryMvSafe();

    if (!isValidPacket(packet)) {
        return false;
    }

    uint8_t buffer[sizeof(MeshAlertPacket)] = {0};
    copyPacketToBuffer(packet, buffer, sizeof(buffer));

    if (!sendEspNowPacket(buffer, sizeof(buffer))) {
        Serial.println("ESP-NOW send failed after retries");
        return false;
    }
    return true;
}

static bool shouldEmitHeartbeat(uint32_t bootCount) {
    if (!gConfig.enableHeartbeat) {
        return false;
    }
    if (gConfig.heartbeatEveryBoots == 0) {
        return false;
    }
    return (bootCount % gConfig.heartbeatEveryBoots) == 0;
}

void setup() {
    Serial.begin(115200);
    delay(100);

    initRtcState();
    ++gBootCount;
    gRtcState.wakeCount = gBootCount;

    pinMode(A0, INPUT);
    if (gConfig.buzzerEnabled && gConfig.buzzerPin >= 0) {
        pinMode(gConfig.buzzerPin, OUTPUT);
        digitalWrite(gConfig.buzzerPin, LOW);
    }
    initEspNow();

    const int rawSmoke = readSmokeAverage(A0, 5);
    const int smokeValue = normalizeSmokeReading(rawSmoke);
    const int tempSensorPin = gConfig.temperatureSensorEnabled ?
        (gConfig.temperatureSensorPin >= 0 ? gConfig.temperatureSensorPin : gConfig.ds18b20Pin) :
        -1;
    const float temperatureC = readTemperatureC(tempSensorPin);
    const bool selfTest = shouldRunSelfTest(gBootCount, gConfig.selfTestIntervalDays);
    const bool alarm = isSensorAlarm(smokeValue,
                                    temperatureC,
                                    gConfig.smokeThreshold,
                                    gConfig.tempThreshold,
                                    gConfig.alarmLevel);
    const int batteryMv = readBatteryMvSafe();
    const bool heartbeat = shouldEmitHeartbeat(gBootCount);
    const bool lowBattery = batteryMv > 0 && batteryMv < gConfig.lowBatteryMv;

    appendReading(gRtcState, smokeValue, temperatureC);
    if (selfTest) {
        gRtcState.lastSelfTest = millis();
    }

    const EventType eventType = (selfTest && !alarm) ? EventType::TEST : EventType::ALERT;
    if (alarm || selfTest) {
        broadcastAlert(smokeValue, temperatureC, eventType);
    } else if (heartbeat) {
        broadcastAlert(smokeValue, temperatureC, EventType::HEARTBEAT);
    } else if (lowBattery) {
        broadcastAlert(smokeValue, temperatureC, EventType::STATUS);
    }

    if (alarm && shouldSoundLocalBuzzer() && gConfig.buzzerEnabled && gConfig.buzzerPin >= 0) {
        tone(gConfig.buzzerPin, 2000, gConfig.buzzerPulseMs);
        delay(gConfig.buzzerPulseMs + 25);
        noTone(gConfig.buzzerPin);
    }

    Serial.printf("Boot=%lu smoke=%d temp=%s alarm=%s selfTest=%s heartbeat=%s lowBattery=%s\n",
                  static_cast<unsigned long>(gBootCount),
                  smokeValue,
                  isnan(temperatureC) ? "nan" : "valid",
                  alarm ? "true" : "false",
                  selfTest ? "true" : "false",
                  heartbeat ? "true" : "false",
                  lowBattery ? "true" : "false");

    prepareDeepSleep(gConfig.deepSleepSeconds > 0 ? gConfig.deepSleepSeconds : kDeepSleepSeconds);
}

void loop() {
    delay(1000);
}

} // namespace subnode
} // namespace emberlink

void setup() {
    emberlink::subnode::setup();
}

void loop() {
    emberlink::subnode::loop();
}
