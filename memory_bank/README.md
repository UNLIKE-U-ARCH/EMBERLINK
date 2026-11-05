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

## Planned feature: Peer Mesh Consensus (PMC)

Peer Mesh Consensus is a planned optional validation layer for EmberLink and is not intended to replace the current working broadcast-only sensor-to-gateway alarm flow.

### Purpose
PMC is intended to add local peer verification to the mesh so nearby sensor nodes can corroborate an event before the gateway escalates a high-confidence emergency response. The design goal is to improve alarm confidence in sparse rural deployments without destabilizing the existing low-power system.

### Core principle
- The legacy path remains the default working flow: sensor detects alarm -> node broadcasts -> gateway dispatches.
- PMC sits above that flow as an opt-in trust layer.
- PMC is only activated through configuration and never forced on by default.
- The gateway remains the final authority for dispatch decisions.

### Planned behavior
1. A node detects a significant alarm condition.
2. If PMC is enabled, the node first emits a PRE_ALERT packet instead of immediately escalating to a full emergency classification.
3. Nearby peer nodes receive the PRE_ALERT and evaluate their own local readings within a short validation window.
4. Each peer that sees a matching or corroborating condition emits a PEER_CONFIRM packet.
5. The gateway tracks the event origin, peer confirmations, and confidence window.
6. If enough peer confirmation is received, the gateway treats the event as higher-confidence and dispatches with stronger confidence logic.
7. If no peer aligns, the system still falls back to the original single-node working path.

### Planned packet additions
PMC requires shared packet extensions, but they are additive and should remain backward-compatible with the present mesh contract.

Planned packet concepts:
- PRE_ALERT: initial local warning broadcast from a node
- PEER_CONFIRM: nearby node confirms a matching threat condition
- RELAY: optional forward of a message when local hop limits permit
- eventSequence: unique ID for a single event cycle
- originNodeId: the originating field node
- relayNodeId: last hop / forwarding node
- hopCount: number of mesh hops taken
- maxHops: maximum number of allowed hops for one event cycle
- confirmationCount: count of peer confirmations for a given event
- timestampUtc: event time for stale-packet filtering

### Planned config additions
The feature will be controlled through the existing node configuration layer.

Planned settings:
- enablePeerMonitoring = false by default
- peerConfirmationWindowMs = short timeout window for peer agreement
- maxPeerHops = 1 or 2 in the initial version
- peerNodeIds[] = static local peer map for a first deployment-safe implementation
- peerConfirmationThreshold = minimum number of confirmations before alarm confidence increases

### Planned gateway behavior
- Accept both old and new packet variants.
- Maintain a pending-event map keyed by origin node and event sequence.
- Count PRE_ALERT and PEER_CONFIRM messages.
- Ignore stale or looped packets using hop count and timestamp checks.
- Keep the normal dispatch pathway operational when PMC is disabled or when peer agreement is absent.

### Planned implementation approach
The first release of PMC should remain intentionally small and deterministic.

Proposed rollout:
1. Add new packet fields and event types without changing legacy behavior.
2. Add a feature flag and local peer list in the node config.
3. Add PRE_ALERT generation on the sub-node when PMC is enabled.
4. Add PEER_CONFIRM generation for nearby nodes during the matching validation window.
5. Add gateway aggregation logic for event confidence scoring.
6. Verify that one-node alerting still works exactly as before.
7. Continue with one-hop static peer confirmation before attempting broader mesh logic.

### Why this is the correct low-risk approach
- It preserves the current working architecture.
- It adds peer validation without forcing a full dynamic route algorithm.
- It keeps the gateway as the final decision point.
- It works with the current low-power ESP32 and ESP-NOW model.
- It avoids unnecessary complexity while still enabling a real local validation layer.

### Planned verification gates
- Gate A: legacy alert path remains functional when PMC is off.
- Gate B: packet parsing works for both old and new packet definitions.
- Gate C: peer confirmation from one nearby node produces a stronger event confidence state.
- Gate D: duplicate or stale peer packets are rejected.
- Gate E: no infinite routing or repeated dispatch occurs after peer confirmation.

### Deployment note
This is a future enhancement, not a current replacement for the deployed base behavior. The initial PMC release should be a gated, opt-in feature with conservative thresholds and limited hop range. Subsequent versions can expand to more dynamic routing only after the validation model is proven on real hardware.
