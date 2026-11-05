#pragma once

// Author attribution: Sourish Raj Gubbala.
#include "../shared/emberlink_types.h"

namespace emberlink {
namespace subnode {

struct Config {
    char nodeId[16] = "LANE-01";
    int smokeThreshold = DEFAULT_SMOKE_THRESHOLD;
    float tempThreshold = DEFAULT_TEMP_THRESHOLD;
    int alarmLevel = static_cast<int>(AlarmLevel::LEVEL_1);
    bool temperatureSensorEnabled = false;
    int temperatureSensorPin = 4;
    int ds18b20Pin = 4;
    bool buzzerEnabled = false;
    int buzzerPin = -1;
    uint32_t buzzerPulseMs = 250;
    uint32_t buzzerCooldownMs = 30000;
    uint32_t deepSleepSeconds = 60;
    uint32_t selfTestIntervalDays = 7;
    uint32_t heartbeatEveryBoots = 12;
    bool enableHeartbeat = true;
    int lowBatteryMv = 3000;

    // Peer Mesh Consensus (PMC) configuration. All defaults keep the existing
    // legacy broadcast path active with no behavior change unless explicitly enabled.
    // The legacy alias keeps older firmware configs compatible while the new flag
    // remains the preferred setting name.
    bool enablePeerMonitoring = false;
    bool peerMonitoringEnabled = false; // legacy compatibility alias
    uint32_t peerConfirmationWindowMs = 3000;
    uint8_t maxPeerHops = 1;
    uint8_t peerConfirmationThreshold = 1;
    uint8_t peerNodeCount = 0;
    char peerNodeIds[4][16] = {
        "", "", "", ""
    };
};

} // namespace subnode
} // namespace emberlink
