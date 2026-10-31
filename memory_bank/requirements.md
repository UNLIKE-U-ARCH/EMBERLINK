<!-- Author attribution: Sourish Raj Gubbala -->
# EmberLink Requirements

## Product requirements
- Build a rural fire safety mesh using ESP32 sub-nodes and a cellular gateway.
- Sub-node uses deep sleep, RTC buffering, and ESP-NOW broadcast.
- Gateway uses ESP-NOW receive plus SIM7670G call and SMS handling.
- Gateway keeps a local configuration portal for setup and status.
- Each physical role gets its own firmware image and upload target.

## Upload model
- Field sub-node units receive the sub-node firmware build.
- Gateway unit receives the gateway firmware build.
- Shared sensor/data logic is reused across both builds.

## Sensor requirements
- Preserve analog smoke ADC normalization.
- Preserve DS18B20 validity checks.
- Preserve the existing alarm logic and threshold behavior.
- Preserve the RTC-backed buffer and weekly self-test behavior.

## Dispatch and command requirements
- Gateway listens for alarm packets over ESP-NOW.
- Gateway sends emergency voice calls using ATD.
- Gateway sends emergency SMS using +CMGS.
- Gateway supports inbound SMS commands: STATUS, TEST, RESET, SILENCE.
- Commands must be handled through a modem command parser and execution layer.
