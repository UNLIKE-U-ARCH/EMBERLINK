<!-- Author attribution: Sourish Raj Gubbala -->
# Phase 5 verification plan

## Goal
Validate the EmberLink system on real hardware before calling it deployment-ready.

## Local validation script
- Run `./phase5_verify.ps1` from the project root to execute the host-side Phase 5 validation checks available in this environment.
- This script validates the sub-node guard logic, packet contract, and gateway config-portal stub in the absence of the ESP32 toolchain.
- It does not replace real ESP32 bench validation; it acts as the available pre-hardware gate.

## Scope
Phase 5 is a verification gate, not a feature expansion phase. The system should only move to broader rollout after both firmware targets behave correctly in the expected field conditions.

## Verification checklist

### 1. Toolchain and build checks
- Confirm the ESP32 Arduino/PlatformIO toolchain is installed and working.
- Build the sub-node firmware target cleanly.
- Build the gateway firmware target cleanly.
- Confirm all shared logic still compiles in the common layer.

### 2. Sub-node bench validation
- Confirm smoke ADC reads are stable and normalized as expected.
- Confirm invalid or out-of-range smoke samples do not produce false alerts.
- Confirm temperature validity checks reject disconnected or invalid sensor values.
- Confirm the RTC ring-buffer is updated correctly across wake cycles.
- Confirm self-test triggers align with the configured cadence.
- Confirm message broadcast occurs only for alert or valid test events.

### 3. Gateway bench validation
- Confirm SIM7670G modem initialization is reliable on UART2.
- Confirm call and SMS dispatch paths are executed only after a valid packet is received.
- Confirm duplicate alerts are suppressed during the cooldown window.
- Confirm silence state does not block status/test reporting and only mutes real dispatch.
- Confirm inbound SMS commands parse correctly for STATUS, TEST, RESET, and SILENCE.
- Confirm the gateway can log and respond to command execution without losing state.

### 4. End-to-end alarm path validation
- One sub-node wakes and reports an alarm packet.
- Gateway receives the ESP-NOW packet.
- Gateway validates the packet schema.
- Gateway sends the emergency SMS and attempts the voice call.
- Gateway stores the last alert state and runtime flags.
- A phone command like STATUS or RESET updates the gateway behavior correctly.

### 5. Field-style readiness checklist
- Confirm power draw is acceptable for deep-sleep battery operation.
- Confirm mesh reliability is stable for a one-hop rural deployment.
- Confirm the emergency number and admin number are correctly configured.
- Confirm local gateway configuration remains accessible and stable.
- Confirm alert logs and command replies are consistent across runs.

## Exit criteria
Phase 5 is complete only when all of the above checks pass on actual hardware in the target deployment model.

## Important rule
No broad rollout should begin until the gateway modem, the field sensor logic, and the shared packet contract are all validated together in an end-to-end test.
