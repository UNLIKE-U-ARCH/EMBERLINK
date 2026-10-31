// Author attribution: Sourish Raj Gubbala.
#include "power.h"

#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include "../stubs/Arduino.h"
#endif
#if __has_include(<esp_sleep.h>)
#include <esp_sleep.h>
#else
#include "../stubs/esp_sleep.h"
#endif

namespace emberlink {
namespace subnode {

void prepareDeepSleep(uint32_t seconds) {
    uint32_t safeSeconds = seconds;
    if (safeSeconds == 0) {
        safeSeconds = 60;
    }
    if (safeSeconds < 15) {
        safeSeconds = 15;
    }
    if (safeSeconds > 86400) {
        safeSeconds = 86400;
    }

    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(safeSeconds) * 1000000ULL);
    esp_deep_sleep_start();
}

bool shouldRunSelfTest(uint32_t bootCount, uint32_t intervalDays) {
    if (bootCount == 0) {
        return false;
    }
    if (intervalDays == 0) {
        return false;
    }

    uint32_t safeInterval = intervalDays;
    if (safeInterval < 1) {
        safeInterval = 7;
    }

    return (bootCount % safeInterval) == 0;
}

} // namespace subnode
} // namespace emberlink
