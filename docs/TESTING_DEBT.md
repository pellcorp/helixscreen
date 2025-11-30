# Test Technical Debt

This document tracks tests that are hidden from the default test run using Catch2's hidden tag mechanism (`[.]`). Hidden tests are excluded from normal test execution but can be run explicitly when needed.

## Overview

Hidden tests exist for various reasons:
- **Crashes/Instability**: Tests that cause crashes (SIGILL, SIGSEGV) during execution or cleanup
- **External Dependencies**: Tests requiring live connections, hardware, or external resources
- **Performance**: Tests that are too slow for normal CI runs
- **Implementation Gaps**: Tests for features not yet fully implemented

**Running hidden tests:**
```bash
# Run specific hidden test
./build/bin/run_tests "[.slow]"

# Run all hidden tests
./build/bin/run_tests "[.]"

# Run hidden tests by category
./build/bin/run_tests "[.integration]"
./build/bin/run_tests "[.ui_integration]"
```

---

## Hidden Test Inventory

### 1. CommandSequencer - Queue Management

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_command_sequencer.cpp:88` |
| **Tags** | `[sequencer][.]` |
| **Symptom** | SIGILL crash during fixture destruction |
| **Root Cause** | Memory corruption or mock cleanup issue - likely race between callback execution and fixture teardown |
| **Priority** | **HIGH** - Blocks testing of core sequencer functionality |
| **Fix Approach** | Investigate SequencerTestFixture destructor, ensure all callbacks are cancelled before mock destruction. May need explicit cleanup ordering or weak_ptr safety. |

---

### 2. Mock Print Phase State Machine Transitions

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_mock_print_simulation.cpp:254` |
| **Tags** | `[mock_print][phase][.]` |
| **Symptom** | SIGSEGV crash during fixture destruction |
| **Root Cause** | Similar to CommandSequencer - mock lifecycle management issue with MockPrintTestFixture |
| **Priority** | **HIGH** - Blocks testing of print phase state machine |
| **Fix Approach** | Same pattern as CommandSequencer. Ensure temperature simulation thread is fully stopped and all callbacks are flushed before fixture destruction. |

---

### 3. Mock Print Speedup Factor Behavior

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_mock_print_simulation.cpp:396` |
| **Tags** | `[mock_print][speedup][.]` |
| **Symptom** | SIGSEGV crash during fixture destruction |
| **Root Cause** | Same underlying issue as phase state machine tests - MockPrintTestFixture lifecycle |
| **Priority** | **MEDIUM** - Important for simulation tuning but not blocking |
| **Fix Approach** | Fix will come with MockPrintTestFixture cleanup improvements (same as #2) |

---

### 4. RibbonGeometry Construction and Destruction

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_gcode_geometry_builder.cpp:153` |
| **Tags** | `[gcode][geometry][ribbon][.]` |
| **Symptom** | Test assertion fails - `normal_cache_ptr` is nullptr |
| **Root Cause** | Cache initialization issue in RibbonGeometry constructor; move semantics may be corrupted |
| **Priority** | **MEDIUM** - Affects G-code geometry rendering tests |
| **Fix Approach** | Review RibbonGeometry constructor, ensure caches are properly initialized. Check move constructor preserves cache pointers correctly. |

---

### 5. RibbonGeometry Move Semantics

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_gcode_geometry_builder.cpp:165` |
| **Tags** | `[gcode][geometry][ribbon][.]` |
| **Symptom** | Test assertion fails - `normal_cache_ptr` is nullptr after move |
| **Root Cause** | Move constructor does not properly transfer cache ownership |
| **Priority** | **MEDIUM** - Related to #4 |
| **Fix Approach** | Implement proper move semantics for cache pointers; may need unique_ptr instead of raw pointers |

---

### 6. MoonrakerClient Concurrent Send Requests

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_moonraker_client_robustness.cpp:97` |
| **Tags** | `[.][moonraker][robustness][concurrent][priority1]` |
| **Symptom** | Mutex lock failed: "Invalid argument" during test cleanup |
| **Root Cause** | Race condition between test teardown and callback execution; callbacks execute after mutex is destroyed |
| **Priority** | **HIGH** - Critical for verifying thread safety |
| **Fix Approach** | Implement proper callback cancellation in MoonrakerClient destructor. Consider using atomic flags or condition variables to ensure all in-flight callbacks complete before destruction. |

---

### 7. MoonrakerClient State Machine Transitions

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_moonraker_client_robustness.cpp:617` |
| **Tags** | `[.][moonraker][robustness][state][priority4]` |
| **Symptom** | `send_jsonrpc` returns -1 instead of 0 when disconnected |
| **Root Cause** | Implementation behavior changed - WebSocket send() fails when not connected |
| **Priority** | **LOW** - Test expectations may be outdated |
| **Fix Approach** | Review expected behavior: should requests be queued when disconnected? Update test or implementation based on design decision. |

---

### 8. MoonrakerClient Stress Test - Sustained Load

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_moonraker_client_robustness.cpp:887` |
| **Tags** | `[moonraker][robustness][stress][.slow]` |
| **Symptom** | Test takes too long for normal CI runs (10+ seconds) |
| **Root Cause** | By design - stress test sends 1000 requests |
| **Priority** | **LOW** - Working correctly, just excluded from fast CI |
| **Fix Approach** | None needed - run explicitly with `[.slow]` tag when performance testing |

---

### 9. WiFi Network Information

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_wifi_manager.cpp:544` |
| **Tags** | `[wifi][networks][.disabled]` |
| **Symptom** | `scan_once()` returns before scan completes |
| **Root Cause** | scan_once() doesn't wait for scan completion - needs async rewrite |
| **Priority** | **MEDIUM** - Affects WiFi testing coverage |
| **Fix Approach** | Rewrite test to use async scan with callback, or add explicit wait (2s delay) for thread completion |

---

### 10. MoonrakerClient Timeout Callbacks Outside Mutex

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_moonraker_client_security.cpp:364` |
| **Tags** | `[moonraker][security][deadlock][issue6][.integration]` |
| **Symptom** | Test requires actual WebSocket connection |
| **Root Cause** | Without connection, send_jsonrpc fails immediately with CONNECTION_ERROR before timeout can occur |
| **Priority** | **MEDIUM** - Security regression test for Issue #6 |
| **Fix Approach** | Set up mock WebSocket server in test infrastructure, or integrate with running Moonraker instance for integration tests |

---

### 11. MoonrakerClient Security Properties Integration

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_moonraker_client_security.cpp:812` |
| **Tags** | `[.][moonraker][security][integration]` |
| **Symptom** | SIGSEGV during client destruction |
| **Root Cause** | Object lifetime issues when destroying client while callbacks are registered/executing |
| **Priority** | **HIGH** - Tests critical security properties |
| **Fix Approach** | Related to concurrent access issues (#6). Fix callback cleanup in destructor. |

---

### 12. Wizard Connection Performance Benchmark

| Field | Value |
|-------|-------|
| **File** | `tests/unit/test_wizard_connection.cpp:327` |
| **Tags** | `[wizard][connection][performance][.benchmark]` |
| **Symptom** | None - test works but is slow |
| **Root Cause** | By design - performance benchmark runs 10000 iterations |
| **Priority** | **LOW** - Working correctly, excluded from fast CI |
| **Fix Approach** | None needed - run explicitly with `[.benchmark]` tag for performance regression testing |

---

### 13-20. Wizard Connection UI Tests (8 tests)

| Field | Value |
|-------|-------|
| **Files** | `tests/unit/test_wizard_connection_ui.cpp:123,141,173,188,214,234,345,395` |
| **Tags** | `[wizard][connection][ui][.ui_integration]` |
| **Symptom** | XML components not registered - tests fail to find widgets |
| **Root Cause** | Test fixture's `ensure_components_registered()` is a stub; XML filesystem driver not set up |
| **Priority** | **MEDIUM** - Blocks UI integration testing |
| **Fix Approach** | Set up XML filesystem driver in test infrastructure, or run with full LVGL initialization |

**Affected tests:**
1. `Connection UI: All widgets exist` (:123)
2. `Connection UI: Input field interaction` (:141)
3. `Connection UI: Test button state` (:173)
4. `Connection UI: Status label updates` (:188)
5. `Connection UI: Navigation buttons` (:214)
6. `Connection UI: Title and progress` (:234)
7. `Connection UI: Input validation feedback` (:345)
8. `Connection UI: Responsive layout` (:395)

---

## Standardized Hidden Tag Conventions

Use these standardized tags for new hidden tests:

| Tag | Meaning | When to Use |
|-----|---------|-------------|
| `[.]` | Generic hidden | Catch-all for hidden tests |
| `[.needs_impl]` | Feature not implemented | Test written before implementation |
| `[.needs_fix]` | Known bug preventing test | Test blocked by identified issue |
| `[.integration]` | Requires external resources | Needs live server, hardware, etc. |
| `[.ui_integration]` | Requires LVGL/UI infrastructure | Needs full UI initialization |
| `[.slow]` | Too slow for normal runs | Stress tests, benchmarks (>5 sec) |
| `[.benchmark]` | Performance measurement | Timing-sensitive tests |
| `[.disabled]` | Temporarily disabled | Awaiting fix or design decision |
| `[.flaky]` | Intermittent failures | Timing-dependent, race conditions |

---

## Action Items (Prioritized)

### HIGH Priority (Blocks Important Testing)

1. **Fix fixture destruction crashes** - CommandSequencer, MockPrint, MoonrakerClient
   - Root cause: Callbacks executing during/after destructor
   - Solution: Implement proper callback cancellation pattern
   - Files: `test_command_sequencer.cpp`, `test_mock_print_simulation.cpp`, `test_moonraker_client_robustness.cpp`

2. **Fix MoonrakerClient concurrent access**
   - Root cause: Mutex lock failures during cleanup
   - Solution: Use atomic flags or condition variables for cleanup synchronization
   - Files: `test_moonraker_client_robustness.cpp`, `test_moonraker_client_security.cpp`

### MEDIUM Priority (Reduces Test Coverage)

3. **Fix RibbonGeometry cache initialization**
   - Root cause: nullptr cache pointers
   - Solution: Review constructor and move semantics
   - File: `test_gcode_geometry_builder.cpp`

4. **Fix WiFi scan_once() async behavior**
   - Root cause: Returns before scan completes
   - Solution: Use async pattern with callback
   - File: `test_wifi_manager.cpp`

5. **Set up UI integration test infrastructure**
   - Root cause: XML components not registered
   - Solution: Initialize LVGL filesystem driver in test setup
   - File: `test_wizard_connection_ui.cpp`

### LOW Priority (Working as Designed)

6. **Document slow/benchmark tests**
   - These work correctly, just excluded from CI
   - Run explicitly: `./build/bin/run_tests "[.slow]"` or `"[.benchmark]"`

7. **Review state machine test expectations**
   - May need to update test or document expected behavior change
   - File: `test_moonraker_client_robustness.cpp:617`

---

## Summary Statistics

| Category | Count |
|----------|-------|
| Total Hidden Tests | 20 |
| Crashes (SIGILL/SIGSEGV) | 7 |
| Requires Integration Setup | 10 |
| Performance/Slow | 2 |
| Design Questions | 1 |

**Test Debt by Priority:**
- HIGH: 7 tests (35%)
- MEDIUM: 11 tests (55%)
- LOW: 2 tests (10%)

---

*Last updated: 2025-11-29*
*Generated by analyzing test files in `tests/unit/`*
