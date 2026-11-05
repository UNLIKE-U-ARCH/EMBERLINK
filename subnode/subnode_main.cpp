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

struct PeerConfirmationCache {
    char originNodeId[16] = "";
    uint32_t sequence = 0;
    uint32_t lastSeenMs = 0;
};

constexpr size_t kPeerConfirmationCacheSize = 8;
static PeerConfirmationCache gPeerConfirmationCache[kPeerConfirmationCacheSize] = {};
static Config gConfig;

static void onPeerPacketReceive(const uint8_t* mac,
                                const uint8_t* incomingData,
                                int len);

static bool isPeerMonitoringEnabled() {
    return gConfig.enablePeerMonitoring || gConfig.peerMonitoringEnabled;
}

static bool isTrustedPeer(const char* nodeId) {
    if (nodeId == nullptr || nodeId[0] == '\0') {
        return false;
    }

    for (uint8_t i = 0; i < gConfig.peerNodeCount; ++i) {
        if (gConfig.peerNodeIds[i][0] != '\0' &&
            strcmp(gConfig.peerNodeIds[i], nodeId) == 0) {
            return true;
        }
    }

    return false;
}

static bool shouldSuppressPeerConfirmation(const MeshAlertPacket& packet) {
    const char* originNode = packet.originNodeId[0] != '\0' ? packet.originNodeId : packet.nodeId;
    if (originNode[0] == '\0' || !isTrustedPeer(originNode)) {
        return true;
    }

    const uint32_t nowMs = millis();
    for (size_t i = 0; i < kPeerConfirmationCacheSize; ++i) {
        if (gPeerConfirmationCache[i].originNodeId[0] == '\0') {
            continue;
        }
        if (strcmp(gPeerConfirmationCache[i].originNodeId, originNode) == 0 &&
            gPeerConfirmationCache[i].sequence == packet.sequence &&
            (nowMs - gPeerConfirmationCache[i].lastSeenMs) < 3000UL) {
            return true;
        }
    }

    for (size_t i = 0; i < kPeerConfirmationCacheSize; ++i) {
        if (gPeerConfirmationCache[i].originNodeId[0] == '\0') {
            strncpy(gPeerConfirmationCache[i].originNodeId, originNode, sizeof(gPeerConfirmationCache[i].originNodeId) - 1);
            gPeerConfirmationCache[i].sequence = packet.sequence;
            gPeerConfirmationCache[i].lastSeenMs = nowMs;
            return false;
        }
    }

    return false;
}

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

    if (isPeerMonitoringEnabled()) {
        esp_now_register_recv_cb(onPeerPacketReceive);
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

static bool sendPeerConfirmation(const MeshAlertPacket& triggeringPacket) {
    if (!isPeerMonitoringEnabled()) {
        return false;
    }

    if (shouldSuppressPeerConfirmation(triggeringPacket)) {
        return false;
    }

    const char* originKey = triggeringPacket.originNodeId[0] != '\0' ?
        triggeringPacket.originNodeId : triggeringPacket.nodeId;
    if (!isTrustedPeer(originKey)) {
        return false;
    }

    MeshAlertPacket packet{};
    strncpy(packet.nodeId, gConfig.nodeId, sizeof(packet.nodeId) - 1);
    strncpy(packet.originNodeId, originKey, sizeof(packet.originNodeId) - 1);
    strncpy(packet.relayNodeId, gConfig.nodeId, sizeof(packet.relayNodeId) - 1);

    packet.magic = RTC_MAGIC;
    packet.version = MESH_PACKET_VERSION;
    packet.sequence = triggeringPacket.sequence;
    packet.timestampUtc = millis() / 1000UL;
    packet.eventType = EventType::PEER_CONFIRM;
    packet.alarmLevel = triggeringPacket.alarmLevel;
    packet.smokeValue = triggeringPacket.smokeValue;
    packet.temperatureC = triggeringPacket.temperatureC;
    packet.batteryMv = readBatteryMvSafe();
    packet.hopCount = triggeringPacket.maxHops > 0 && triggeringPacket.hopCount < triggeringPacket.maxHops ?
        static_cast<uint8_t>(triggeringPacket.hopCount + 1) : triggeringPacket.hopCount;
    packet.maxHops = triggeringPacket.maxHops;
    packet.confirmationCount = 1;

    if (!isValidPacket(packet)) {
        return false;
    }

    uint8_t buffer[sizeof(MeshAlertPacket)] = {0};
    copyPacketToBuffer(packet, buffer, sizeof(buffer));
    return sendEspNowPacket(buffer, sizeof(buffer));
}

static void onPeerPacketReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    if (incomingData == nullptr || len <= 0) {
        return;
    }

    const bool peerMonitoringEnabled = isPeerMonitoringEnabled();
    if (!peerMonitoringEnabled) {
        return;
    }

    MeshAlertPacket received{};
    copyBufferToPacket(incomingData, static_cast<size_t>(len), received);
    if (!isValidPacket(received)) {
        return;
    }

    if (strcmp(received.nodeId, gConfig.nodeId) == 0) {
        return;
    }

    if (received.eventType != EventType::PRE_ALERT && received.eventType != EventType::PEER_CONFIRM) {
        return;
    }

    const char* originKey = received.originNodeId[0] != '\0' ? received.originNodeId : received.nodeId;
    if (strcmp(originKey, gConfig.nodeId) == 0) {
        return;
    }

    if (!isTrustedPeer(received.nodeId) && !isTrustedPeer(originKey)) {
        return;
    }

    if (received.maxHops > 0 && received.hopCount >= received.maxHops) {
        return;
    }

    Serial.printf("Peer monitoring: observed %s from %s; sending confirmation\n",
                  received.eventType == EventType::PRE_ALERT ? "PRE_ALERT" : "PEER_CONFIRM",
                  received.nodeId);
    sendPeerConfirmation(received);
}

static bool broadcastAlert(int smokeValue,
                           float temperatureC,
                           EventType eventType,
                           const char* originNodeId = nullptr,
                           const char* relayNodeId = nullptr,
                           uint8_t hopCount = 0,
                           uint8_t maxHops = 0,
                           uint8_t confirmationCount = 0) {
    MeshAlertPacket packet;
    strncpy(packet.nodeId, gConfig.nodeId, sizeof(packet.nodeId) - 1);
    if (originNodeId != nullptr && originNodeId[0] != '\0') {
        strncpy(packet.originNodeId, originNodeId, sizeof(packet.originNodeId) - 1);
    } else {
        strncpy(packet.originNodeId, gConfig.nodeId, sizeof(packet.originNodeId) - 1);
    }
    if (relayNodeId != nullptr && relayNodeId[0] != '\0') {
        strncpy(packet.relayNodeId, relayNodeId, sizeof(packet.relayNodeId) - 1);
    }
    packet.sequence = ++gSequence;
    packet.timestampUtc = millis() / 1000UL;
    packet.eventType = eventType;
    packet.alarmLevel = static_cast<AlarmLevel>(gConfig.alarmLevel);
    packet.smokeValue = smokeValue;
    packet.temperatureC = temperatureC;
    packet.batteryMv = readBatteryMvSafe();
    packet.hopCount = hopCount;
    packet.maxHops = maxHops;
    packet.confirmationCount = confirmationCount;

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

    const bool peerMonitoringEnabled = isPeerMonitoringEnabled();
    const EventType eventType = (selfTest && !alarm) ? EventType::TEST : EventType::ALERT;
    if (alarm && peerMonitoringEnabled) {
        broadcastAlert(smokeValue,
                       temperatureC,
                       EventType::PRE_ALERT,
                       gConfig.nodeId,
                       nullptr,
                       0,
                       gConfig.maxPeerHops,
                       0);
    }

    if (alarm || selfTest) {
        broadcastAlert(smokeValue, temperatureC, eventType, gConfig.nodeId, nullptr, 0, gConfig.maxPeerHops, 0);
    } else if (heartbeat) {
        broadcastAlert(smokeValue, temperatureC, EventType::HEARTBEAT, gConfig.nodeId, nullptr, 0, gConfig.maxPeerHops, 0);
    } else if (lowBattery) {
        broadcastAlert(smokeValue, temperatureC, EventType::STATUS, gConfig.nodeId, nullptr, 0, gConfig.maxPeerHops, 0);
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
