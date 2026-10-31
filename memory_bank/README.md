<!-- Author attribution: Sourish Raj Gubbala -->
# EmberLink Memory Bank

This folder is the project-local memory bank for the EmberLink refactor.

## Purpose
- Capture the architecture decisions for the split between sub-node and gateway firmware.
- Preserve the current smoke/temperature logic as the canonical sensor behavior.
- Track implementation constraints for deep sleep, ESP-NOW, and SIM7670G dispatch.
- Keep the original prototype untouched while the new firmware is built.

## Core rules
- Legacy file [EURTH SRC.txt](EURTH%20SRC.txt) is a reference only.
- New work must be created in a new EmberLink firmware file and not by editing the legacy prototype.
- Sub-node firmware is sensor-only and power-optimized.
- Gateway firmware is the only node with local configuration and cellular dispatch.
- The command set is: STATUS, TEST, RESET, SILENCE.

## Build split and upload model
- Sub-node firmware is uploaded only to the field sensor nodes.
- Gateway firmware is uploaded only to the central dispatch node.
- The two images are different builds, not a single universal firmware image.
- Shared logic is reused by both targets: packet schema, alarm rules, DS18B20 validity checks, smoke calibration logic, and RTC buffer structure.

## Firmware responsibilities
### Sub-node firmware
- ESP32 target
- No Wi-Fi manager
- No HTTP cloud posting
- No web configuration portal
- ESP-NOW broadcast mesh transport
- Deep sleep with RTC memory buffering
- Weekly self-test routine
- Reads smoke + temperature and evaluates alarm conditions

### Gateway firmware
- ESP32 target
- SIM7670G modem on UART2 (GPIO16 RX, GPIO17 TX)
- ESP-NOW receiver callback
- Local AP + captive portal config
- Voice call dispatch using ATD
- SMS dispatch using +CMGS
- Incoming SMS command parser and executor

## Shared logic contract
- Shared packet format for mesh events between sub-node and gateway
- Packet fields include node ID, event type, timestamp, sensor values, alarm state, and sequence number
- Gateway validates packet integrity before dispatching calls or SMS
- Sub-node logic stays focused on field detection and energy conservation
- Gateway logic stays focused on network dispatch and local configuration

## Implementation principle
- Use shared common logic where behavior is identical
- Use target-specific firmware where the runtime role differs
- Keep the hardware roles explicit to avoid cross-loading the wrong binary into the wrong unit

## Phase 5 verification and rollout status
- Phase 5 is the verification gate after the Phase 4 hardening pass.
- The next work is not feature expansion; it is hardware validation, end-to-end alarm verification, and rollout readiness checks.
- Before a deployment readout is considered valid, the project must confirm the ESP32 toolchain, sub-node wake path, packet flow, and gateway modem path on real hardware.

## Phase 1 + 2 cleanup status
- Shared packet validation was tightened so the gateway only accepts trusted mesh payloads.
- Sub-node logic was corrected to avoid double-normalizing smoke values and to send a proper test event for self-checks.
- The node now follows a cleaner wake -> sample -> evaluate -> transmit -> deep sleep flow, aligned with low-power field deployment.

## Current verified state snapshot (2026-10-20)
- The project is in the handoff between Phase 5 verification and Phase 6 hardware proof.
- The gateway runtime has been restored to the hardened split-architecture version, including modem-state checks, duplicate alert suppression, SMS/voice dispatch fallback, inbound SMS command handling, and startup integration with the gateway web-config stub.
- The host-side validation gate has passed with the project's local harness: PHASE5_GAPS_FIXED_OK and PHASE5_HOST_VALIDATION_OK.
- The project remains blocked on real ESP32 hardware proof for the final deployment claim; the remaining proof is on actual sensor and gateway boards, not just the host validation harness.
- Tomorrow's session should focus on: toolchain proof on hardware, sub-node deep-sleep and RTC behavior, gateway modem initialization and SMS dispatch, and end-to-end alarm verification from node to gateway to emergency action.

## Phase 2 active status
- The field node is now structured around a real wake/sense/alert sequence with dedicated sensor and power helpers.
- Smoke ADC sampling and sleep control are separated from the main runtime loop to match the intended deep-sleep architecture.
- The next changes are focused on real temperature input wiring, RTC recovery logic, and self-test scheduling before the gateway dispatch path is expanded further.
