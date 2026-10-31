// Author attribution: Sourish Raj Gubbala.
#include "gateway_web_config.h"

#include <string.h>

namespace emberlink {
namespace gateway {

namespace {
void copyString(char* target, size_t targetSize, const char* source) {
    if (target == nullptr || targetSize == 0) {
        return;
    }
    memset(target, 0, targetSize);
    if (source == nullptr) {
        return;
    }
    const size_t length = strnlen(source, targetSize - 1);
    memcpy(target, source, length);
    target[length] = '\0';
}
}

bool GatewayWebConfig::ensureDefaults() {
    if (settings.ssid[0] == '\0') {
        copyString(settings.ssid, sizeof(settings.ssid), "EMBERLINK-GATEWAY");
    }
    if (settings.password[0] == '\0') {
        copyString(settings.password, sizeof(settings.password), "emberlink123");
    }
    if (settings.emergencyNumber[0] == '\0') {
        copyString(settings.emergencyNumber, sizeof(settings.emergencyNumber), "+1234567890");
    }
    if (settings.adminNumber[0] == '\0') {
        copyString(settings.adminNumber, sizeof(settings.adminNumber), "+1234567890");
    }
    return true;
}

void GatewayWebConfig::begin(const char* ssid, const char* password) {
    resetToDefaults();
    if (ssid != nullptr && ssid[0] != '\0') {
        copyString(settings.ssid, sizeof(settings.ssid), ssid);
    }
    if (password != nullptr && password[0] != '\0') {
        copyString(settings.password, sizeof(settings.password), password);
    }
    portalReady = true;
    Serial.println("Gateway config portal initialized and ready for local operational settings.");
}

void GatewayWebConfig::loop() {
    if (!portalReady) {
        return;
    }
}

bool GatewayWebConfig::isPortalReady() const {
    return portalReady;
}

const GatewayWebConfig::Settings& GatewayWebConfig::getSettings() const {
    return settings;
}

void GatewayWebConfig::applySettings(const Settings& newSettings) {
    settings = newSettings;
    ensureDefaults();
}

void GatewayWebConfig::resetToDefaults() {
    memset(&settings, 0, sizeof(settings));
    settings.ssid[0] = '\0';
    settings.password[0] = '\0';
    settings.emergencyNumber[0] = '\0';
    settings.adminNumber[0] = '\0';
    settings.modemEnabled = true;
    copyString(settings.ssid, sizeof(settings.ssid), "EMBERLINK-GATEWAY");
    copyString(settings.password, sizeof(settings.password), "emberlink123");
    copyString(settings.emergencyNumber, sizeof(settings.emergencyNumber), "+1234567890");
    copyString(settings.adminNumber, sizeof(settings.adminNumber), "+1234567890");
}

} // namespace gateway
} // namespace emberlink
