<!-- Author attribution: Sourish Raj Gubbala -->
# EmberLink Architecture Memory

## Sub-node firmware
- ESP32 target
- No Wi-Fi manager
- No HTTP cloud posting
- ESP-NOW broadcast mesh transport
- Deep sleep with RTC buffer
- Weekly self-test routine
- Sensor loop remains the same as the current logic
- Role: detect fire conditions and transmit alerts to the gateway

## Gateway firmware
- ESP32 target
- SIM7670G modem on UART2 (GPIO16 RX, GPIO17 TX)
- ESP-NOW receiver callback
- Local AP + captive portal config
- Voice call dispatch using ATD
- SMS dispatch using +CMGS
- Incoming SMS command parser and executor
- Role: receive mesh alarms and dispatch emergency actions

## Shared contract
- Shared packet format for mesh events between sub-node and gateway
- Packet fields include node ID, event type, timestamp, sensor values, alarm state, and sequence number
- Gateway validates packet integrity before dispatching calls or SMS
- Shared logic includes raw smoke normalization, DS18B20 validity checks, RTC ring-buffer structure, and alarm decision logic

## Role separation
- Sub-node firmware is uploaded to sensing units only.
- Gateway firmware is uploaded to the central modem/control unit only.
- Firmware is split by responsibility, not by a single universal binary.
- Shared code is kept central, but the runtime behavior differs by target

## Phase 1 + 2 cleanup update
- The shared mesh packet contract now includes a version field and a validation helper to reject malformed or mismatched payloads.
- The sub-node runtime was corrected to prevent double-normalization and to distinguish alert packets from self-test packets.
- The firmware architecture remains split into low-power sensing nodes and a modem-capable dispatch gateway, with the cleanup focused on preserving that separation without placeholder logic.

## Phase 2 implementation update
- The sub-node runtime has moved beyond a placeholder loop and now follows a wake -> sample -> evaluate -> broadcast -> deep sleep pattern.
- Sensor and power responsibilities were separated to support the field-node constraints more cleanly.
- The gateway is still the dispatch and modem layer, while the sub-node remains optimized for low-power sensing and mesh alerting.

## Phase 3 implementation update
- The gateway modem layer now initializes SIM7670G with SMS mode and message-notify configuration.
- ESP-NOW receiving is validated against the shared packet contract before any emergency call or SMS is triggered.
- Alert handling distinguishes normal alarms from test packets and can mute dispatches with a silence state.
- Inbound modem commands are parsed for STATUS, TEST, RESET, and SILENCE, allowing phone-based supervision of the gateway state.

## Phase 4 hardening update
- The gateway action path is separated from raw packet receive logic so message generation, duplicate suppression, and modem-side failure handling are explicit.
- The sub-node path has guard rails for invalid ADC and sleep edge cases, keeping the low-power sensor runtime resilient before hardware validation.
- The shared packet and RTC contract remains the canonical source of truth for both builds.

## Phase 5 verification update
- The next gate is real-device verification of the modem, ESP-NOW link, and sensor wake path.
- Bench validation must confirm smoke sampling, temperature validity, RTC persistence, self-test cadence, gateway SMS dispatch, and the full alarm chain from node to modem.
- Field-style rollout should only begin after the above checks pass in a controlled environment.

## Phase 6 hardware validation has started
- The project has moved past pure logic hardening and into the first real ESP32 proof pass.
- The focus is no longer feature growth but proving the actual wake -> sample -> broadcast -> receive -> dispatch path on hardware.
- The system remains intentionally production-minded without becoming overly strict: retry logic, tolerant validation, and safe fallback behavior are preferred over brittle fail-fast logic.
- The Phase 6 checklist is documented in [phase6_hardware_validation.md](phase6_hardware_validation.md).
- The core goals for this phase are toolchain proof, sub-node deep-sleep validation, gateway modem validation, and end-to-end alarm confirmation on real hardware.

## Current state snapshot (2026-10-20)
- The gateway runtime is intentionally restored to the current hardened state, with modem state tracking, duplicate alert suppression, safe dispatch fallback, and inbound command handling.
- Host-side verification is currently green using the local validation harness: PHASE5_GAPS_FIXED_OK and PHASE5_HOST_VALIDATION_OK.
- The remaining gate is hardware validation: actual ESP32 board proof, SIM7670G modem behavior, ESP-NOW delivery, and end-to-end alarm response.
- This means the project is not yet considered hardware-deployment ready, but the architecture and logic are stable enough for the next real-board proof pass.
