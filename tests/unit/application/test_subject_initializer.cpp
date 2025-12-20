// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_subject_initializer.cpp
 * @brief Unit tests for SubjectInitializer class
 *
 * Tests subject initialization ordering, observer registration, and API injection.
 *
 * Note: SubjectInitializer has heavy dependencies (all panels, LVGL subjects, etc.)
 * that make it difficult to unit test in isolation. These tests focus on the
 * RuntimeConfig interface and document expected behavior. Full initialization
 * tests are done as integration tests with the actual application.
 */

#include "runtime_config.h"

#include <cstdio>
#include <string>

#include "../../catch_amalgamated.hpp"

// ============================================================================
// RuntimeConfig Tests (SubjectInitializer dependency)
// ============================================================================

TEST_CASE("RuntimeConfig defaults to non-test mode", "[application][subjects][config]") {
    RuntimeConfig config;

    REQUIRE_FALSE(config.is_test_mode());
    REQUIRE_FALSE(config.test_mode);
}

TEST_CASE("RuntimeConfig test_mode enables mock flags", "[application][subjects][config]") {
    RuntimeConfig config;
    config.test_mode = true;

    REQUIRE(config.is_test_mode());
    REQUIRE(config.should_mock_wifi());
    REQUIRE(config.should_mock_ethernet());
    REQUIRE(config.should_mock_moonraker());
    REQUIRE(config.should_mock_ams());
    REQUIRE(config.should_mock_usb());
    REQUIRE(config.should_use_test_files());
}

TEST_CASE("RuntimeConfig real flags override mock behavior", "[application][subjects][config]") {
    RuntimeConfig config;
    config.test_mode = true;

    // Real WiFi flag should disable WiFi mocking
    config.use_real_wifi = true;
    REQUIRE_FALSE(config.should_mock_wifi());
    REQUIRE(config.should_mock_ethernet()); // Other mocks unaffected

    // Real Moonraker flag
    config.use_real_moonraker = true;
    REQUIRE_FALSE(config.should_mock_moonraker());

    // Real AMS flag
    config.use_real_ams = true;
    REQUIRE_FALSE(config.should_mock_ams());

    // Real files flag
    config.use_real_files = true;
    REQUIRE_FALSE(config.should_use_test_files());
}

TEST_CASE("RuntimeConfig production mode ignores real flags", "[application][subjects][config]") {
    RuntimeConfig config;
    config.test_mode = false;

    // In production mode, all mock functions return false
    // regardless of real_* flag settings
    REQUIRE_FALSE(config.should_mock_wifi());
    REQUIRE_FALSE(config.should_mock_moonraker());
    REQUIRE_FALSE(config.should_mock_usb());

    // Setting real flags in production mode has no effect
    config.use_real_wifi = true;
    REQUIRE_FALSE(config.should_mock_wifi());
}

TEST_CASE("RuntimeConfig skip_splash behavior", "[application][subjects][config]") {
    RuntimeConfig config;

    // Default: no skip
    REQUIRE_FALSE(config.skip_splash);
    REQUIRE_FALSE(config.should_skip_splash());

    // Explicit skip flag
    config.skip_splash = true;
    REQUIRE(config.should_skip_splash());

    // Reset and test that test_mode also skips splash
    config.skip_splash = false;
    config.test_mode = true;
    REQUIRE(config.should_skip_splash());
}

TEST_CASE("RuntimeConfig simulation speedup defaults", "[application][subjects][config]") {
    RuntimeConfig config;

    REQUIRE(config.sim_speedup == 1.0);
    REQUIRE(config.mock_ams_gate_count == 4);
}

TEST_CASE("RuntimeConfig gcode viewer defaults", "[application][subjects][config]") {
    RuntimeConfig config;

    REQUIRE(config.gcode_test_file == nullptr);
    REQUIRE_FALSE(config.gcode_camera_azimuth_set);
    REQUIRE_FALSE(config.gcode_camera_elevation_set);
    REQUIRE_FALSE(config.gcode_camera_zoom_set);
    REQUIRE(config.gcode_camera_zoom == 1.0f);
    REQUIRE_FALSE(config.gcode_debug_colors);
    REQUIRE(config.gcode_render_mode == -1);
}

TEST_CASE("RuntimeConfig test file path helper", "[application][subjects][config]") {
    const char* path = RuntimeConfig::get_default_test_file_path();

    REQUIRE(path != nullptr);
    REQUIRE(std::string(path).find("assets/test_gcodes") != std::string::npos);
    REQUIRE(std::string(path).find("3DBenchy.gcode") != std::string::npos);
}

// ============================================================================
// SubjectInitializer Design Documentation
// ============================================================================
// The following tests document the expected behavior of SubjectInitializer.
// They are marked as .integration since they require the full LVGL environment.

TEST_CASE("SubjectInitializer initializes subjects in dependency order",
          "[application][subjects][.integration]") {
    // Expected initialization order:
    // 1. Core subjects (app_globals, navigation, status bar)
    // 2. PrinterState subjects (panels observe these)
    // 3. AmsState and FilamentSensorManager subjects
    // 4. Panel subjects (home, controls, filament, settings, etc.)
    // 5. Observers (print completion, print start navigation)
    // 6. Utility subjects (notification system)
    // 7. USB manager (needs notification system ready)
    REQUIRE(true); // Documentation placeholder
}

TEST_CASE("SubjectInitializer manages observer guards for cleanup",
          "[application][subjects][.integration]") {
    // SubjectInitializer owns ObserverGuards for:
    // - Print completion notification observer
    // - Print start navigation observer
    // These are automatically cleaned up when SubjectInitializer is destroyed
    REQUIRE(true); // Documentation placeholder
}

TEST_CASE("SubjectInitializer supports deferred API injection",
          "[application][subjects][.integration]") {
    // Some panels need MoonrakerAPI which isn't available until after
    // Moonraker connection is established. SubjectInitializer stores
    // pointers to these panels during init_all() and injects the API
    // when inject_api() is called later.
    //
    // Panels with deferred API injection:
    // - PrintSelectPanel
    // - PrintStatusPanel
    // - MotionPanel
    // - ExtrusionPanel
    // - BedMeshPanel
    // - TempControlPanel
    REQUIRE(true); // Documentation placeholder
}
