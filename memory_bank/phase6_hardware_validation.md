<!-- Author attribution: Sourish Raj Gubbala -->
# Phase 6 hardware validation plan

## Goal
Validate the EmberLink system on actual ESP32 hardware so the project can move from architecture-hardening into demonstrable field behavior.

## Scope
Phase 6 is the first real hardware proof pass. It focuses on proving that the sensor node and gateway behave correctly under actual runtime conditions without adding new feature scope.

## Immediate execution checklist

### 1. ESP32 toolchain and build proof
- Confirm the Arduino/PlatformIO environment is installed and working.
- Build the sub-node target cleanly.
- Build the gateway target cleanly.
- Confirm both target builds still compile against the shared packet contract defined in [../shared/emberlink_types.h](../shared/emberlink_types.h) and [../shared/emberlink_shared.h](../shared/emberlink_shared.h).

### 2. Sub-node hardware proof
- Confirm the RTC wake pattern is stable across reset and sleep cycles.
- Confirm the node wakes, samples, and broadcasts a valid alert or test packet.
- Confirm invalid smoke values are rejected without generating false alarms.
- Confirm temperature sanity checks behave correctly with disconnected or noisy values.
- Confirm the self-test cadence remains stable and does not run too often.
- Confirm deep-sleep entry causes the device to recover without corrupting state.

### 3. Gateway hardware proof
- Confirm the modem initializes reliably over UART2 and enters a ready state.
- Confirm status and command responses remain readable even after minor modem noise.
- Confirm ESP-NOW receive callback reads valid packets and ignores malformed payloads.
- Confirm duplicate alert suppression works without blocking legitimate re-alerts.
- Confirm SMS dispatch and voice-call dispatch remain tolerant under transient failures.

### 4. End-to-end proof
- Trigger a sub-node alarm packet.
- Confirm the gateway receives the packet over ESP-NOW.
- Confirm the gateway validates the packet.
- Confirm the gateway sends the intended emergency notification.
- Confirm the gateway accepts phone-side commands such as STATUS and RESET.

## Bench checklist in execution order

### A. Build proof
- [ ] Build sub-node firmware
- [ ] Build gateway firmware
- [ ] Confirm shared library compatibility
- [ ] Record any toolchain-specific issues before runtime testing

### B. Sub-node bench checks
- [ ] Device boots in sensor-only mode
- [ ] RTC state survives a wake cycle
- [ ] Smoke ADC is stable and valid under test conditions
- [ ] Invalid smoke values are rejected
- [ ] Temperature sanity checks reject disconnected/noisy values
- [ ] Alert or test packet is transmitted using the shared mesh format
- [ ] Node re-enters deep sleep without corrupting state

### C. Gateway bench checks
- [ ] Modem initializes over UART2
- [ ] Gateway reaches modem ready state
- [ ] SMS and command parser respond correctly
- [ ] ESP-NOW receive callback accepts valid packets
- [ ] Malformed packets are ignored safely
- [ ] Duplicate alert suppression works correctly
- [ ] Dispatch path remains alive after a transient failure

### D. End-to-end checks
- [ ] Trigger sub-node alert packet
- [ ] Confirm gateway receives the packet
- [ ] Confirm packet validation passes
- [ ] Confirm emergency SMS or voice dispatch occurs
- [ ] Confirm STATUS and RESET commands work from the phone
- [ ] Confirm alarm path recovers cleanly after a command or transient modem issue

## Production-minded rule
The project should prioritize operational resilience over strict perfection:
- retry transient faults instead of hard-failing
- log abnormal conditions instead of crashing the whole system
- keep thresholds practical for rural sensor conditions
- preserve the safety flow without making the runtime brittle

## Exit criteria for Phase 6
Phase 6 is complete when all of the above checks pass on real ESP32 hardware and the system can demonstrate the full packet path from sensor to modem without requiring a code-level redesign.
