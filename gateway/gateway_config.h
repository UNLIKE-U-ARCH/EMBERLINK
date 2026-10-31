#pragma once

// Author attribution: Sourish Raj Gubbala.
#include <stdint.h>

namespace emberlink {
namespace gateway {

struct Config {
    char gatewayName[16] = "GATEWAY-01";
    char emergencyNumber[20] = "+1234567890";
    char adminNumber[20] = "+1234567890";
    char apSsid[32] = "EMBERLINK-GATEWAY";
    char apPassword[32] = "emberlink123";
    bool modemEnabled = true;
    int uartBaud = 115200;
};

} // namespace gateway
} // namespace emberlink
