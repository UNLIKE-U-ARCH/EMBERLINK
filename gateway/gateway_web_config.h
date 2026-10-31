#pragma once

// Author attribution: Sourish Raj Gubbala.
#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include "../stubs/Arduino.h"
#endif

namespace emberlink {
namespace gateway {

class GatewayWebConfig {
public:
    struct Settings {
        char ssid[32] = "EMBERLINK-GATEWAY";
        char password[32] = "emberlink123";
        char emergencyNumber[20] = "+1234567890";
        char adminNumber[20] = "+1234567890";
        bool modemEnabled = true;
    };

    void begin(const char* ssid, const char* password);
    void loop();
    bool isPortalReady() const;
    const Settings& getSettings() const;
    void applySettings(const Settings& newSettings);
    void resetToDefaults();

private:
    bool portalReady = false;
    Settings settings;
    bool ensureDefaults();
};

} // namespace gateway
} // namespace emberlink
