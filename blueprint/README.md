<!-- Author attribution: Sourish Raj Gubbala -->
# EMBERLINK Blueprint

This folder holds the working blueprint for the EmberLink system build, validation, and rollout process.

## Purpose
- Define the exact architecture for the sensor-node and gateway split.
- Show how the system operates from wake-up to emergency dispatch.
- Capture the step-by-step implementation path for building and validating the project.
- Keep the project state legible for future sessions and hardware proof work.

## Files in this folder
- [emberlink_work_blueprint.md](emberlink_work_blueprint.md) — full system blueprint, graphical flow, and implementation plan.

## Core principles
- Sub-node firmware is sensor-only and low-power.
- Gateway firmware is the only node with modem and dispatch logic.
- Shared packet contracts are the canonical source of truth.
- The system must be validated in stages: logic, hardware bench, field proof.

## Mission summary
The EmberLink system is a rural ESP-NOW mesh fire safety network with a central gateway that receives alerts, validates them, and dispatches emergency SMS and voice calls while supporting inbound control SMS.

## Quick view
- Detect fire condition on sub-node
- Send ESP-NOW alert packet
- Validate packet on gateway
- Suppress duplicates and handle silence state
- Send emergency SMS and call
- Support reverse SMS commands like STATUS, TEST, RESET, and SILENCE
