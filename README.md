<!-- Author attribution: Sourish Raj Gubbala -->
# EmberLink

EmberLink is a rural fire-safety mesh and emergency-dispatch system built around ESP32 devices. The architecture is split into two firmware targets:

- a low-power sensor node that monitors smoke and temperature
- a modem-enabled gateway that receives alerts and dispatches emergency communications

The system is designed for scalable field deployment in rural or semi-connected areas where a cellular gateway can serve as the command and dispatch layer while sub-nodes remain energy-efficient and focused on sensing.

## Project vision

EmberLink aims to combine:

- low-power ESP32 sensing nodes
- ESP-NOW mesh communication
- RTC-backed deep-sleep behavior
- local gateway coordination
- SIM7670G-based emergency SMS and voice dispatch
- SMS command handling for remote control and status checks

This project is built around practical field reliability rather than a single monolithic firmware image. The system intentionally separates responsibilities so the sensor layer remains lean and the gateway remains the only node with modem and configuration responsibilities.

## System architecture

### 1. Sub-node firmware

The sub-node is the field sensing unit. It is optimized for low power and long battery life.

Core responsibilities:

- read smoke and temperature data
- validate sensor readings and reject invalid values
- decide whether an alarm condition exists
- broadcast a mesh alert packet over ESP-NOW
- optionally trigger a very short local buzzer pulse for nearby awareness
- enter deep sleep with RTC-backed state
- run a periodic self-test routine

Important constraints:

- no Wi-Fi manager
- no cloud posting layer
- no local portal or modem logic
- no emergency dispatch role
- local buzzer is a brief optional assist only; the gateway remains the primary emergency-response node

### 2. Gateway firmware

The gateway is the network control and dispatch node. It is the only unit that should be responsible for modem and local control behavior.

Core responsibilities:

- receive ESP-NOW alarm packets
- validate packet integrity before taking action
- suppress duplicate alerts within a cooldown window
- send emergency SMS alerts
- attempt emergency voice call dispatch
- manage inbound SMS commands like STATUS, TEST, RESET, and SILENCE
- maintain local gateway configuration and operational state

Important constraints:

- gateway is the only node with cellular dispatch logic
- gateway is the only node that maintains local setup and runtime control
- all alarm actions pass through a guarded validation layer before dispatch

### 3. Shared logic

The shared layer holds common logic used by both firmware targets.

It includes:

- packet schema and versioning
- RTC buffer structure
- smoke normalization rules
- temperature validation helpers
- alarm decision logic
- packet serialization and deserialization helpers

This contract keeps the two builds consistent and avoids drift between the sensor and gateway logic.

## Functional requirements

### Sensor and sub-node requirements

- capture smoke readings from the analog input path
- validate readings against practical sanity thresholds
- reject disconnected or invalid temperature data
- normalize smoke readings before alarm evaluation
- store wake-cycle state in RTC memory
- transmit valid alerts and test packets over ESP-NOW
- sleep efficiently between wake cycles
- optionally emit a brief local buzzer pulse when a real alarm is detected, without turning the node into a continuous siren

### Gateway and dispatch requirements

- receive valid mesh alerts from the sensor layer
- reject malformed or mismatched packets
- send emergency SMS messages with alert details
- attempt emergency voice calls when configured
- suppress duplicate alerts to avoid spam dispatches
- accept command SMS messages from the configured admin number
- support the command set: STATUS, TEST, RESET, SILENCE

### Shared operational requirements

- maintain the split-role architecture across both firmware images
- keep code and runtime responsibilities explicitly separated
- keep safety logic practical and resilient rather than brittle
- prefer retry and graceful fallback over hard failure when a transient issue occurs

## Supported command set

The gateway supports the following simple command set:

- STATUS: return runtime state and last alert status
- TEST: confirm modem and gateway responsiveness
- RESET: clear alert state and reset key runtime flags
- SILENCE: mute live emergency dispatch while preserving logging and state

These commands are intended for admin or operator use and are handled at the gateway modem layer.

## Current project status

The project is currently in the Phase 6 hardware-validation stage.

This means the architecture is already implemented and the logic is largely hardened, but the remaining critical work is real-device proof on ESP32 hardware.

Completed work includes:

- role separation between sub-node and gateway
- shared packet schema and validation logic
- sub-node wake/sense/broadcast/sleep flow
- gateway receive and dispatch flow
- duplicate suppression logic
- modem command handling and state tracking
- memory bank documentation and project-phase tracking

Remaining work includes:

- real ESP32 build validation on hardware
- sub-node RTC and deep-sleep proof
- sensor validation on actual devices
- gateway modem validation on UART2
- end-to-end mesh-to-dispatch verification
- demo polish and field-readiness tuning

## Phase roadmap

### Phase 1 and 2: architecture and cleanup

- separated sub-node and gateway responsibilities
- fixed packet contract consistency
- cleaned up low-power sensing logic and field-node design

### Phase 3: modem and command path

- added modem initialization flow
- implemented SMS and voice dispatch logic
- added inbound command parsing and execution

### Phase 4: hardening

- improved duplicate suppression and dispatch safety
- stabilized sensor and deep-sleep edge cases
- improved overall runtime resilience

### Phase 5: verification gate

- host-side checks passed for the pre-hardware validation path
- hardware proof remained the key remaining gate

### Phase 6: hardware validation

- validate build behavior on actual ESP32 hardware
- test wake/sleep logic, sensing, and dispatch path in real runtime conditions
- verify end-to-end system behavior before exhibit/demo polish

## Design principles

The project follows a few guiding principles:

- sensors stay focused on sensing
- gateways stay focused on dispatch and control
- shared logic remains canonical and central
- the system should be operationally resilient, not unnecessarily rigid
- hardware proof is required before calling the system deployment-ready

## Practical implementation philosophy

The system is intentionally designed to be robust in a rural safety context without becoming overly complicated or brittle.

That means:

- retry transient modem faults instead of failing hard
- tolerate minor sensor noise without generating false alarms
- keep the safety path active and usable during a temporary communication issue
- preserve practical field deployment behavior rather than idealized laboratory perfection

## Summary

EmberLink is a practical, rural-focused fire safety and dispatch project based on ESP32 hardware. It uses a split architecture to separate low-power field sensing from cellular dispatch and command handling. The project is already at a strong prototype and near-production level in logic and structure, and it is now in the concrete hardware-validation phase required to prove the system works in real conditions.

## Repository layout

- [shared/emberlink_types.h](shared/emberlink_types.h)
- [shared/emberlink_shared.h](shared/emberlink_shared.h)
- [subnode/subnode_main.cpp](subnode/subnode_main.cpp)
- [subnode/sensors.cpp](subnode/sensors.cpp)
- [subnode/power.cpp](subnode/power.cpp)
- [gateway/gateway_main.cpp](gateway/gateway_main.cpp)
- [gateway/gateway_config.h](gateway/gateway_config.h)
- [gateway/gateway_web_config.h](gateway/gateway_web_config.h)
- [gateway/gateway_web_config.cpp](gateway/gateway_web_config.cpp)
- [memory_bank/](memory_bank/)
- [phase5_verify.ps1](phase5_verify.ps1)

## Notes

This README reflects the current architecture and project state as documented in the memory bank. The project has passed the available pre-hardware validation gate, but real ESP32 hardware validation remains the essential proof step before calling the system fully ready for deployment.
