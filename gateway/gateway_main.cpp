// Author attribution: Sourish Raj Gubbala.
#include <Arduino.h>
#if __has_include(<WiFi.h>)
#include <WiFi.h>
#endif
#if __has_include(<esp_now.h>)
#include <esp_now.h>
#endif
#if __has_include(<HardwareSerial.h>)
#include <HardwareSerial.h>
#endif

#include "gateway_config.h"
#include "gateway_web_config.h"
#include "../shared/emberlink_shared.h"

namespace emberlink {
namespace gateway {

static Config gConfig;
static bool dispatchEmergencyAlert(const MeshAlertPacket& packet);

namespace {
constexpr uint8_t kModemRxPin = 16;
constexpr uint8_t kModemTxPin = 17;
constexpr uint32_t kModemBaud = 115200;

enum class ModemState : uint8_t {
    BOOT = 0,
    READY = 1,
    SMS_MODE = 2,
    ERROR = 3
};

struct GatewayRuntimeState {
    bool alarmSilenced = false;
    uint32_t lastAlertUtc = 0;
    uint32_t lastCommandUtc = 0;
    uint32_t lastDispatchFailureUtc = 0;
    uint32_t lastAlertSequence = 0;
    String lastAlertNodeId = "";
    ModemState modemState = ModemState::BOOT;
    bool modemReady = false;
};

struct PeerEventRecord {
    char originNodeId[16] = "";
    uint32_t sequence = 0;
    uint32_t firstSeenUtc = 0;
    uint8_t confirmations = 0;
    char confirmingPeers[4][16] = {"", "", "", ""};
    bool resolved = false;
};

constexpr uint32_t kDispatchCooldownSec = 60;
constexpr size_t kPeerEventCapacity = 8;
static PeerEventRecord gPeerEvents[kPeerEventCapacity] = {};

static const char* getOriginNodeId(const MeshAlertPacket& packet) {
    if (packet.originNodeId[0] != '\0') {
        return packet.originNodeId;
    }
    return packet.nodeId;
}

static bool hasPeerConfirmed(const PeerEventRecord& record, const char* peerNodeId) {
    if (peerNodeId == nullptr || peerNodeId[0] == '\0') {
        return false;
    }

    for (uint8_t i = 0; i < 4; ++i) {
        if (record.confirmingPeers[i][0] != '\0' &&
            strcmp(record.confirmingPeers[i], peerNodeId) == 0) {
            return true;
        }
    }

    return false;
}

static PeerEventRecord* findOrCreatePeerEventRecord(const MeshAlertPacket& packet) {
    const char* originNodeId = getOriginNodeId(packet);
    if (originNodeId == nullptr || originNodeId[0] == '\0') {
        return nullptr;
    }

    for (size_t i = 0; i < kPeerEventCapacity; ++i) {
        if (gPeerEvents[i].originNodeId[0] != '\0' &&
            strcmp(gPeerEvents[i].originNodeId, originNodeId) == 0 &&
            gPeerEvents[i].sequence == packet.sequence) {
            return &gPeerEvents[i];
        }
    }

    for (size_t i = 0; i < kPeerEventCapacity; ++i) {
        if (gPeerEvents[i].originNodeId[0] == '\0') {
            strncpy(gPeerEvents[i].originNodeId, originNodeId, sizeof(gPeerEvents[i].originNodeId) - 1);
            gPeerEvents[i].sequence = packet.sequence;
            gPeerEvents[i].firstSeenUtc = packet.timestampUtc;
            gPeerEvents[i].confirmations = 0;
            for (uint8_t j = 0; j < 4; ++j) {
                gPeerEvents[i].confirmingPeers[j][0] = '\0';
            }
            gPeerEvents[i].resolved = false;
            return &gPeerEvents[i];
        }
    }

    return nullptr;
}

static void handlePeerConsensusPacket(const MeshAlertPacket& packet) {
    if (packet.eventType == EventType::PRE_ALERT) {
        PeerEventRecord* record = findOrCreatePeerEventRecord(packet);
        if (record != nullptr) {
            Serial.printf("PMC pre-alert recorded for node=%s sequence=%u\n",
                          record->originNodeId,
                          record->sequence);
        }
        return;
    }

    if (packet.eventType == EventType::PEER_CONFIRM) {
        PeerEventRecord* record = findOrCreatePeerEventRecord(packet);
        if (record == nullptr || record->resolved) {
            return;
        }

        if (packet.nodeId[0] == '\0' || strcmp(packet.nodeId, record->originNodeId) == 0) {
            return;
        }

        const uint32_t ageSec = packet.timestampUtc - record->firstSeenUtc;
        if (ageSec > (gConfig.peerConfirmationWindowMs / 1000UL)) {
            Serial.printf("PMC confirmation expired for %s\n", record->originNodeId);
            return;
        }

        if (hasPeerConfirmed(*record, packet.nodeId)) {
            return;
        }

        for (uint8_t i = 0; i < 4; ++i) {
            if (record->confirmingPeers[i][0] == '\0') {
                strncpy(record->confirmingPeers[i], packet.nodeId, sizeof(record->confirmingPeers[i]) - 1);
                break;
            }
        }

        record->confirmations = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            if (record->confirmingPeers[i][0] != '\0') {
                record->confirmations += 1;
            }
        }

        Serial.printf("PMC peer confirmation for %s count=%u threshold=%u\n",
                      record->originNodeId,
                      record->confirmations,
                      gConfig.peerConfirmationThreshold);

        if (record->confirmations >= gConfig.peerConfirmationThreshold) {
            record->resolved = true;
            MeshAlertPacket promoted = packet;
            strncpy(promoted.nodeId, record->originNodeId, sizeof(promoted.nodeId) - 1);
            strncpy(promoted.originNodeId, record->originNodeId, sizeof(promoted.originNodeId) - 1);
            promoted.eventType = EventType::ALERT;
            promoted.confirmationCount = record->confirmations;
            Serial.printf("PMC consensus reached for %s; dispatching stronger alert\n", record->originNodeId);
            dispatchEmergencyAlert(promoted);
        }
    }
}

String normalizeCommandText(const String& raw) {
    String normalized = raw;
    normalized.trim();
    normalized.toUpperCase();
    normalized.replace("\r", " ");
    normalized.replace("\n", " ");
    normalized.replace("\t", " ");
    while (normalized.indexOf("  ") >= 0) {
        normalized.replace("  ", " ");
    }
    return normalized;
}

bool parseSmsCommand(const String& text, String& command) {
    const String normalized = normalizeCommandText(text);
    if (normalized.indexOf("STATUS") >= 0) {
        command = "STATUS";
        return true;
    }
    if (normalized.indexOf("TEST") >= 0) {
        command = "TEST";
        return true;
    }
    if (normalized.indexOf("RESET") >= 0) {
        command = "RESET";
        return true;
    }
    if (normalized.indexOf("SILENCE") >= 0) {
        command = "SILENCE";
        return true;
    }
    return false;
}

String extractSenderFromModemLine(const String& text) {
    const int firstQuote = text.indexOf('"');
    if (firstQuote < 0) {
        return "";
    }
    const int secondQuote = text.indexOf('"', firstQuote + 1);
    if (secondQuote <= firstQuote) {
        return "";
    }
    return text.substring(firstQuote + 1, secondQuote);
}

String extractMessageBodyFromModemLine(const String& text) {
    const int firstNewline = text.indexOf('\n');
    if (firstNewline < 0) {
        return "";
    }
    String body = text.substring(firstNewline + 1);
    body.trim();
    return body;
}
}

static GatewayRuntimeState gGatewayState;

String resolveAlertNumber() {
    if (gConfig.emergencyNumber[0] != '\0' && strlen(gConfig.emergencyNumber) > 4) {
        return String(gConfig.emergencyNumber);
    }
    if (gConfig.adminNumber[0] != '\0' && strlen(gConfig.adminNumber) > 4) {
        return String(gConfig.adminNumber);
    }
    return "";
}

bool shouldSuppressDuplicateAlert(const MeshAlertPacket& packet, uint32_t nowSec) {
    if (packet.eventType != EventType::ALERT) {
        return false;
    }

    const String currentNode = String(packet.originNodeId[0] != '\0' ? packet.originNodeId : packet.nodeId);
    if (currentNode.length() == 0) {
        return false;
    }

    const bool sameOrigin = gGatewayState.lastAlertNodeId == currentNode;
    const bool sameSequence = packet.sequence != 0 && packet.sequence == gGatewayState.lastAlertSequence;
    const bool withinWindow = nowSec - gGatewayState.lastAlertUtc < kDispatchCooldownSec;

    return sameOrigin && (sameSequence || withinWindow);
}

class ModemController {
public:
    ModemController() = default;

    void begin() {
        gGatewayState.modemState = ModemState::BOOT;
        gGatewayState.modemReady = false;

        Serial2.begin(kModemBaud, SERIAL_8N1, kModemRxPin, kModemTxPin);
        delay(500);
        flushModemInput();

        if (!sendAtCommand("AT\r", "OK", 2000)) {
            gGatewayState.modemState = ModemState::ERROR;
            return;
        }
        if (!sendAtCommand("ATE0\r", "OK", 2000)) {
            gGatewayState.modemState = ModemState::ERROR;
            return;
        }
        if (!sendAtCommand("AT+CMGF=1\r", "OK", 2000)) {
            gGatewayState.modemState = ModemState::ERROR;
            return;
        }
        if (!sendAtCommand("AT+CNMI=2,2,0,0,0\r", "OK", 2000)) {
            gGatewayState.modemState = ModemState::ERROR;
            return;
        }

        gGatewayState.modemState = ModemState::SMS_MODE;
        gGatewayState.modemReady = true;
    }

    bool ensureModemReady() {
        if (gGatewayState.modemReady) {
            return true;
        }
        Serial.println("Modem not ready; gateway modem path is still in boot or error state");
        return false;
    }

    bool sendAtCommand(const char* command, const char* expected, uint32_t timeoutMs) {
        flushModemInput();
        Serial2.print(command);
        String response;
        return readUntilExpected(expected, response, timeoutMs);
    }

    bool readUntilExpected(const char* expected, String& response, uint32_t timeoutMs) {
        const uint32_t start = millis();
        response = "";

        while (millis() - start < timeoutMs) {
            while (Serial2.available()) {
                const char c = static_cast<char>(Serial2.read());
                response += c;

                if (response.indexOf("ERROR") >= 0) {
                    Serial.printf("Modem reported ERROR waiting for '%s': %s\n", expected, response.c_str());
                    return false;
                }

                if (response.indexOf(expected) >= 0) {
                    return true;
                }
            }
            delay(10);
        }

        Serial.printf("Modem response timeout waiting for '%s': %s\n", expected, response.c_str());
        return false;
    }

    bool sendSms(const String& number, const String& message) {
        if (!ensureModemReady()) {
            return false;
        }
        if (number.length() == 0 || message.length() == 0) {
            return false;
        }

        String response;
        const String command = "AT+CMGS=\"" + number + "\"\r";
        Serial2.print(command);

        if (!readUntilExpected(">", response, 5000)) {
            Serial.println("SMS prompt not received");
            return false;
        }

        Serial2.print(message);
        Serial2.write((uint8_t)26);
        const bool ok = readUntilExpected("OK", response, 10000);

        if (!ok) {
            Serial.printf("SMS send failed for %s\n", number.c_str());
        }
        return ok;
    }

    bool dialVoiceCall(const String& number) {
        if (!ensureModemReady()) {
            return false;
        }
        if (number.length() == 0) {
            return false;
        }

        String response;
        Serial2.print("ATD" + number + ";\r");
        const bool ok = readUntilExpected("OK", response, 8000);

        if (!ok) {
            Serial.printf("Voice call failed for %s\n", number.c_str());
        }
        return ok;
    }

    bool handleInboundCommand(const String& sender, const String& text) {
        if (sender.length() == 0) {
            return false;
        }

        String command;
        if (!parseSmsCommand(text, command)) {
            sendSms(sender, "EMBERLINK: UNKNOWN COMMAND");
            return false;
        }

        gGatewayState.lastCommandUtc = millis() / 1000UL;

        if (command == "STATUS") {
            String status = "EMBERLINK STATUS: online, modem=" +
                            String(gGatewayState.modemReady ? "READY" : "BOOT") +
                            ", silenced=" + String(gGatewayState.alarmSilenced ? "YES" : "NO") +
                            ", lastNode=" + gGatewayState.lastAlertNodeId +
                            ", lastAlertSec=" + String(gGatewayState.lastAlertUtc);
            sendSms(sender, status);
            return true;
        }

        if (command == "TEST") {
            sendSms(sender, "EMBERLINK TEST: gateway modem ready");
            return true;
        }

        if (command == "RESET") {
            gGatewayState.alarmSilenced = false;
            gGatewayState.lastAlertNodeId = "";
            sendSms(sender, "EMBERLINK RESET: gateway state cleared");
            return true;
        }

        if (command == "SILENCE") {
            gGatewayState.alarmSilenced = true;
            sendSms(sender, "EMBERLINK SILENCE: alarm dispatch muted");
            return true;
        }

        sendSms(sender, "EMBERLINK ERROR: unsupported command");
        return false;
    }

    void pollForIncomingSms() {
        static String modemBuffer;

        while (Serial2.available()) {
            const char c = static_cast<char>(Serial2.read());
            modemBuffer += c;

            if (c == '\n') {
                const String line = modemBuffer;

                if (line.indexOf("+CMTI:") >= 0) {
                    const int commaPos = line.indexOf(',');
                    if (commaPos >= 0) {
                        const String storageIndex = line.substring(commaPos + 1);
                        String trimmed = storageIndex;
                        trimmed.trim();
                        if (trimmed.length() > 0) {
                            Serial2.print("AT+CMGR=");
                            Serial2.print(trimmed);
                            Serial2.print("\r");
                        }
                    }
                }

                if (line.indexOf("+CMGR:") >= 0) {
                    const String sender = extractSenderFromModemLine(line);
                    const String body = extractMessageBodyFromModemLine(line);
                    if (sender.length() > 0 && body.length() > 0) {
                        handleInboundCommand(sender, body);
                    }
                }

                modemBuffer = "";
            }
        }
    }

private:
    void flushModemInput() {
        while (Serial2.available()) {
            Serial2.read();
        }
    }
};

static ModemController gModem;

static bool dispatchEmergencyAlert(const MeshAlertPacket& packet) {
    const uint32_t nowSec = millis() / 1000UL;
    const String alertDestination = resolveAlertNumber();

    if (packet.eventType == EventType::TEST) {
        gGatewayState.lastAlertUtc = nowSec;
        gGatewayState.lastAlertNodeId = String(packet.nodeId);
        const String target = alertDestination.length() > 0 ? alertDestination : String(gConfig.adminNumber);
        const bool testSmsSent = gModem.sendSms(target,
                                               "EMBERLINK TEST: node=" + String(packet.nodeId) + " reported a self-check");
        if (!testSmsSent) {
            Serial.println("Test packet SMS failed to send");
        }
        return true;
    }

    if (packet.eventType == EventType::HEARTBEAT) {
        Serial.printf("Heartbeat received from %s battery=%d\n",
                      packet.nodeId,
                      packet.batteryMv);
        const String target = alertDestination.length() > 0 ? alertDestination : String(gConfig.adminNumber);
        if (target.length() > 0) {
            gModem.sendSms(target,
                           "EMBERLINK HEARTBEAT: node=" + String(packet.nodeId) +
                           " battery=" + String(packet.batteryMv) + "mV");
        }
        return true;
    }

    if (packet.eventType == EventType::STATUS) {
        Serial.printf("Status packet received from %s battery=%d\n",
                      packet.nodeId,
                      packet.batteryMv);
        const String target = alertDestination.length() > 0 ? alertDestination : String(gConfig.adminNumber);
        if (target.length() > 0) {
            gModem.sendSms(target,
                           "EMBERLINK STATUS: node=" + String(packet.nodeId) +
                           " battery=" + String(packet.batteryMv) + "mV");
        }
        return true;
    }

    if (shouldSuppressDuplicateAlert(packet, nowSec)) {
        Serial.printf("Duplicate alert suppressed for node %s within %lu seconds\n",
                      packet.nodeId,
                      static_cast<unsigned long>(kDispatchCooldownSec));
        return true;
    }

    if (gGatewayState.alarmSilenced) {
        gGatewayState.lastAlertUtc = nowSec;
        gGatewayState.lastAlertNodeId = String(packet.nodeId);
        const String target = alertDestination.length() > 0 ? alertDestination : String(gConfig.adminNumber);
        const bool silenceInfoSent = gModem.sendSms(target,
                                                   "EMBERLINK SILENCED: alert from " + String(packet.nodeId) + " was logged but not dispatched");
        if (!silenceInfoSent) {
            Serial.println("Silence notification SMS failed to send");
        }
        return true;
    }

    gGatewayState.lastAlertUtc = nowSec;
    gGatewayState.lastAlertNodeId = String(packet.originNodeId[0] != '\0' ? packet.originNodeId : packet.nodeId);
    gGatewayState.lastAlertSequence = packet.sequence;

    if (alertDestination.length() == 0) {
        Serial.println("No valid dispatch destination configured; alert was logged but not dispatched");
        return false;
    }

    const String sms = "EMBERLINK ALERT: node=" + String(packet.nodeId) +
                       " smoke=" + String(packet.smokeValue) +
                       " temp=" + String(packet.temperatureC, 2) +
                       " level=" + String(static_cast<int>(packet.alarmLevel));

    const bool alertSmsSent = gModem.sendSms(alertDestination, sms);
    if (!alertSmsSent) {
        gGatewayState.lastDispatchFailureUtc = nowSec;
        Serial.println("Emergency SMS failed to send; continuing with voice fallback");
    }

    const bool callPlaced = gModem.dialVoiceCall(alertDestination);
    if (!callPlaced) {
        gGatewayState.lastDispatchFailureUtc = nowSec;
        Serial.println("Emergency voice call failed to start; gateway remains in tolerant fallback mode");
    }

    return alertSmsSent || callPlaced;
}

static void onEspNowReceive(const uint8_t* mac, const uint8_t* incomingData, int len) {
    if (incomingData == nullptr || len <= 0) {
        return;
    }

    MeshAlertPacket packet;
    copyBufferToPacket(incomingData, static_cast<size_t>(len), packet);
    if (!isValidPacket(packet)) {
        return;
    }

    Serial.printf("Alert received from %s: smoke=%d temp=%.2f level=%d event=%d\n",
                  packet.nodeId,
                  packet.smokeValue,
                  packet.temperatureC,
                  static_cast<int>(packet.alarmLevel),
                  static_cast<int>(packet.eventType));

    if ((gConfig.enablePeerMonitoring || gConfig.peerMonitoringEnabled) &&
        (packet.eventType == EventType::PRE_ALERT || packet.eventType == EventType::PEER_CONFIRM)) {
        handlePeerConsensusPacket(packet);
        return;
    }

    dispatchEmergencyAlert(packet);
}

void setup() {
    Serial.begin(115200);
    delay(200);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(gConfig.apSsid, gConfig.apPassword);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW gateway init failed");
        return;
    }

    esp_now_register_recv_cb(onEspNowReceive);

    GatewayWebConfig webConfig;
    webConfig.begin(gConfig.apSsid, gConfig.apPassword);

    gModem.begin();
    Serial.println("Gateway ready");
}

void loop() {
    gModem.pollForIncomingSms();

    if (Serial2.available()) {
        while (Serial2.available()) {
            Serial.write(static_cast<uint8_t>(Serial2.read()));
        }
    }
    delay(50);
}

} // namespace gateway
} // namespace emberlink

void setup() {
    emberlink::gateway::setup();
}

void loop() {
    emberlink::gateway::loop();
}