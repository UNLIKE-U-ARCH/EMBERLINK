#include <iostream>
#include "Arduino.h"
#include "subnode/sensors.h"
#include "subnode/power.h"
#include "gateway/gateway_web_config.h"
#include "shared/emberlink_shared.h"

int main() {
    const float temp = emberlink::subnode::readTemperatureC(4);
    if (temp != DEVICE_DISCONNECTED_C) {
        std::cout << "TEMP_GUARD_BAD=" << temp << std::endl;
        return 10;
    }

    if (!emberlink::subnode::shouldRunSelfTest(14, 7)) {
        std::cout << "SELFTEST_GUARD_BAD" << std::endl;
        return 20;
    }

    emberlink::gateway::GatewayWebConfig portal;
    portal.begin("EMBERLINK-GATEWAY", "emberlink123");
    if (!portal.isPortalReady()) {
        std::cout << "PORTAL_BAD" << std::endl;
        return 30;
    }

    emberlink::MeshAlertPacket packet;
    packet.magic = emberlink::RTC_MAGIC;
    packet.version = emberlink::MESH_PACKET_VERSION;
    packet.nodeId[0] = 'N';
    packet.nodeId[1] = '1';
    packet.nodeId[2] = '\0';
    packet.eventType = emberlink::EventType::ALERT;
    packet.alarmLevel = emberlink::AlarmLevel::LEVEL_2;
    packet.smokeValue = 220;
    packet.temperatureC = 41.5f;
    if (!emberlink::isValidPacket(packet)) {
        std::cout << "PACKET_VALIDATION_BAD" << std::endl;
        return 40;
    }

    std::cout << "PHASE5_GAPS_FIXED_OK" << std::endl;
    return 0;
}
