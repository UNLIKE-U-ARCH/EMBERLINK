#pragma once

#include <stdint.h>

#define WIFI_STA 1
#define WIFI_AP 2

class WiFiClass {
public:
    void mode(int) {}
    void disconnect() {}
    bool softAP(const char* ssid, const char* password = nullptr) {
        (void)ssid;
        (void)password;
        return true;
    }
};

extern WiFiClass WiFi;
