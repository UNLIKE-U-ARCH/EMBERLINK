#pragma once

// Author attribution: Sourish Raj Gubbala.
#include <stdint.h>

namespace emberlink {
namespace subnode {

void prepareDeepSleep(uint32_t seconds);
bool shouldRunSelfTest(uint32_t bootCount, uint32_t intervalDays);

} // namespace subnode
} // namespace emberlink
