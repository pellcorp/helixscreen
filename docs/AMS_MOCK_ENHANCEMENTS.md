# AMS Mock Backend Enhancement Plan

**Status:** Phase 1 COMPLETE, Phases 2-3 NOT STARTED (lower priority polish)
**Last Updated:** 2025-12-17
**Consolidated from:** `AMS_MOCK_PHASE2_PLAN.md` + `AMS_MOCK_REALISM_PLAN.md`

## Overview

Enhancements to `AmsBackendMock` for realistic AMS/MMU simulation. Enables thorough UI testing, error handling validation, and demo scenarios without physical hardware.

> **Note:** Lower priority work. Core AMS (Phases 0-4.6) is complete. These mock enhancements improve automated testing and demos but don't block user-facing features.

---

## Current State (Phase 1 Complete)

### What Works
- **Multi-phase operations**: Load/unload show HEATING→LOADING→CHECKING sequences
- **Timing with speedup**: Integrates with `--sim-speed` for fast testing
- **Timing variance**: ±20% variation for realistic feel
- **Path segment progression**: Animates through SPOOL→PREP→LANE→HUB→OUTPUT→TOOLHEAD→NOZZLE
- **Bypass mode**: Enable/disable virtual bypass
- **Dryer simulation**: Temperature ramping, countdown timer, cool-down phase
- **Spoolman integration**: Enriches slot info from mock Spoolman data
- **Tool mapping**: T0-Tn to slot mapping

### Environment Variables (Current)

| Variable | Default | Description |
|----------|---------|-------------|
| `HELIX_AMS_GATES` | 4 | Number of slots in mock |
| `HELIX_MOCK_DRYER` | 0 | Enable dryer simulation |
| `HELIX_MOCK_DRYER_SPEED` | 60 | Dryer time multiplier |

---

## Phase 2: Error Testing & Recovery UI (NOT STARTED)

**Goal:** Enable automatic testing of error recovery flows.

### 2.1 Random Failure Injection

| Failure | Trigger Point | UI Response | Recovery |
|---------|---------------|-------------|----------|
| Filament jam | LOADING at HUB/TOOLHEAD | Error modal | "Retry" or "Cancel" |
| Sensor timeout | CHECKING phase | Error modal | Auto-retry (3x) then manual |
| Tip forming fail | FORMING_TIP phase | Error modal | "Retry with higher temp" |
| Communication error | Any phase | Toast + modal | "Reconnect" |

```cpp
struct FailureConfig {
    float jam_probability = 0.0f;
    float sensor_fail_probability = 0.0f;
    float tip_fail_probability = 0.0f;
    int max_auto_retries = 3;
};
```

### 2.2 Error Recovery Modal

Files: `ui_xml/ams_error_modal.xml`, updates to `src/ui_panel_ams.cpp`

### 2.3 Pause/Resume Support

```cpp
AmsError pause_operation();
AmsError resume_operation();
```

---

## Phase 3: Advanced Features (NOT STARTED)

### 3.1 Multi-Unit Support
Simulate multiple AMS units (e.g., 2x Box Turtle = 8 slots).
```
HELIX_AMS_UNITS=4,4,4  # Three 4-slot units
```

### 3.2 Enhanced Dryer Simulation
Humidity tracking, power consumption stats.

### 3.3 Filament Runout Simulation
Test mid-print runout handling UI.

---

## Priority

1. **Phase 2.1 + 2.2** - Highest value for testing error UI
2. **Phase 2.3** - Nice-to-have
3. **Phase 3.x** - Only if needed for specific testing
