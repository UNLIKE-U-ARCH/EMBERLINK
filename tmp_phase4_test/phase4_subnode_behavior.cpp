#include <math.h>
#include <iostream>
#include "Arduino.h"
#include "subnode/sensors.h"
#include "subnode/power.h"
#include "shared/emberlink_shared.h"

int main() {
    const int smoke = emberlink::subnode::readSmokeAverage(-1, 5);
    if (smoke != emberlink::CALIBRATION) {
        std::cout << "SMOKE_BAD=" << smoke << std::endl;
        return 10;
    }

    const float temp = emberlink::subnode::readTemperatureC(4);
    if (!isfinite(temp) || temp <= -100.0f) {
        std::cout << "TEMP_BAD=" << temp << std::endl;
        return 20;
    }

    if (!emberlink::subnode::shouldRunSelfTest(10080u, 7u, 60u)) {
        std::cout << "SELFTEST_BAD" << std::endl;
        return 30;
    }

    return 0;
}
