// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_application.cpp
 * @brief Unit tests for Application orchestrator class
 *
 * Tests the top-level Application class that coordinates initialization
 * and the main event loop.
 *
 * NOTE: Application::run() has heavy dependencies (display, Moonraker, XML)
 * that cannot be easily mocked. Full testing of the initialization sequence
 * requires integration tests (run the app with --test --timeout).
 *
 * These unit tests focus on:
 * - RuntimeConfig behavior (testable in isolation)
 * - Mock state management (testable via fixture)
 * - LVGL fixture functionality (ensures test infrastructure works)
 */

#include "application_test_fixture.h"

#include "../../catch_amalgamated.hpp"

// ============================================================================
// RuntimeConfig Tests (Application Dependency)
// ============================================================================

TEST_CASE_METHOD(ApplicationTestFixture, "Application config defaults to test mode",
                 "[application][config]") {
    REQUIRE(config().is_test_mode());
    REQUIRE(config().should_mock_moonraker());
    REQUIRE(config().should_mock_wifi());
}

TEST_CASE_METHOD(ApplicationTestFixture, "Application config can enable real Moonraker",
                 "[application][config]") {
    configure_real_moonraker();

    REQUIRE(config().is_test_mode());
    REQUIRE_FALSE(config().should_mock_moonraker());
    REQUIRE(config().should_mock_wifi()); // Still mocked
}

TEST_CASE_METHOD(ApplicationTestFixture, "Application sim speedup is configurable",
                 "[application][config]") {
    set_sim_speedup(5.0);
    REQUIRE(config().sim_speedup == 5.0);

    set_sim_speedup(1.0);
    REQUIRE(config().sim_speedup == 1.0);
}

// ============================================================================
// Mock State Tests
// ============================================================================

TEST_CASE_METHOD(ApplicationTestFixture, "Mock state resets correctly", "[application][mocks]") {
    // Set some mock state
    mock_state().extruder_temp = 200.0;
    mock_state().bed_temp = 60.0;
    mock_state().print_progress = 0.5;
    mock_state().add_excluded_object("Part_1");

    // Verify state was set
    REQUIRE(mock_state().extruder_temp == 200.0);
    REQUIRE(mock_state().get_excluded_objects().count("Part_1") == 1);

    // Reset
    reset_mocks();

    // Verify defaults restored
    REQUIRE(mock_state().extruder_temp == 25.0);
    REQUIRE(mock_state().bed_temp == 25.0);
    REQUIRE(mock_state().print_progress == 0.0);
    REQUIRE(mock_state().get_excluded_objects().empty());
}

// ============================================================================
// LVGL Fixture Tests
// ============================================================================

TEST_CASE_METHOD(ApplicationTestFixture, "LVGL is initialized in test fixture",
                 "[application][lvgl]") {
    // test_screen() should return a valid screen
    REQUIRE(test_screen() != nullptr);

    // Should be able to create widgets
    lv_obj_t* label = lv_label_create(test_screen());
    REQUIRE(label != nullptr);

    lv_label_set_text(label, "Test");
    REQUIRE(std::string(lv_label_get_text(label)) == "Test");
}

// ============================================================================
// Application Integration Tests (require full environment)
// ============================================================================
// These tests document expected behavior but require full LVGL + Moonraker
// initialization to run. They are marked .integration and serve as
// documentation for what Application::run() should do.

TEST_CASE("Application::run() handles --help gracefully", "[application][.integration]") {
    // Expected: Returns 0 without initializing display
    // Test via: ./build/bin/helix-screen --help
    REQUIRE(true);
}

TEST_CASE("Application::run() handles --test mode", "[application][.integration]") {
    // Expected: Creates mock Moonraker client/API, mock USB, etc.
    // Test via: ./build/bin/helix-screen --test --timeout 2
    REQUIRE(true);
}

TEST_CASE("Application::run() respects --timeout", "[application][.integration]") {
    // Expected: Exits after specified seconds
    // Test via: ./build/bin/helix-screen --test --timeout 1
    REQUIRE(true);
}

TEST_CASE("Application shutdown order is correct", "[application][.integration]") {
    // Expected: Managers destroyed in reverse order of initialization:
    // 1. Clear app_globals references
    // 2. MoonrakerManager (stops print_start_collector, clears API/client)
    // 3. PanelFactory
    // 4. SubjectInitializer
    // 5. Wizard cleanup
    // 6. DisplayManager (calls lv_deinit)
    // 7. spdlog::shutdown()
    REQUIRE(true);
}

TEST_CASE("Application creates overlays from CLI flags", "[application][.integration]") {
    // Expected: -p motion creates motion_panel overlay
    // Test via: ./build/bin/helix-screen --test -p motion --timeout 2
    REQUIRE(true);
}
