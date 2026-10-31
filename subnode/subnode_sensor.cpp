#include "subnode_config.h"

#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include "../stubs/Arduino.h"
#endif

#include "../shared/emberlink_shared.h"

namespace emberlink {
namespace subnode {

int readSmokeRawAverage() {
    const int count = 5;
    int samples[count] = {0};
    for (int i = 0; i < count; ++i) {
        samples[i] = analogRead(A0);
        delay(10);
    }
    return computeAverage(samples, count);
}

bool readStructuralState(int smokeThreshold,
                        float tempThreshold,
                        AlarmLevel alarmLevel,
                        int& smokeValue,
                        float& temperatureC) {
    const int rawAverage = readSmokeRawAverage();
    smokeValue = normalizeSmokeReading(rawAverage);
    temperatureC = NAN;

    return isAlarmCondition(smokeValue, temperatureC, smokeThreshold, tempThreshold, alarmLevel);
}

} // namespace subnode
} // namespace emberlink
