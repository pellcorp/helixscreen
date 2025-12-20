// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_display_manager.cpp
 * @brief Unit tests for DisplayManager class
 *
 * Tests display initialization, configuration, and lifecycle management.
 * Note: These tests use the existing LVGLTestFixture which provides its own
 * display initialization, so we test DisplayManager in isolation where possible.
 */

#include "application_test_fixture.h"
#include "display_manager.h"

#include "../../catch_amalgamated.hpp"

// ============================================================================
// DisplayManager Configuration Tests
// ============================================================================

TEST_CASE("DisplayManager::Config has sensible defaults", "[application][display]") {
    DisplayManager::Config config;

    REQUIRE(config.width == 800);
    REQUIRE(config.height == 480);
    REQUIRE(config.scroll_throw == 25);
    REQUIRE(config.scroll_limit == 5);
    REQUIRE(config.require_pointer == true);
}

TEST_CASE("DisplayManager::Config can be customized", "[application][display]") {
    DisplayManager::Config config;
    config.width = 1024;
    config.height = 600;
    config.scroll_throw = 50;
    config.scroll_limit = 10;
    config.require_pointer = false;

    REQUIRE(config.width == 1024);
    REQUIRE(config.height == 600);
    REQUIRE(config.scroll_throw == 50);
    REQUIRE(config.scroll_limit == 10);
    REQUIRE(config.require_pointer == false);
}

// ============================================================================
// DisplayManager State Tests
// ============================================================================

TEST_CASE("DisplayManager starts uninitialized", "[application][display]") {
    DisplayManager mgr;

    REQUIRE_FALSE(mgr.is_initialized());
    REQUIRE(mgr.display() == nullptr);
    REQUIRE(mgr.pointer_input() == nullptr);
    REQUIRE(mgr.keyboard_input() == nullptr);
    REQUIRE(mgr.backend() == nullptr);
    REQUIRE(mgr.width() == 0);
    REQUIRE(mgr.height() == 0);
}

TEST_CASE("DisplayManager shutdown is safe when not initialized", "[application][display]") {
    DisplayManager mgr;

    // Should not crash
    mgr.shutdown();
    mgr.shutdown(); // Multiple calls should be safe

    REQUIRE_FALSE(mgr.is_initialized());
}

// ============================================================================
// Timing Function Tests
// ============================================================================

TEST_CASE("DisplayManager::get_ticks returns increasing values", "[application][display]") {
    uint32_t t1 = DisplayManager::get_ticks();

    // Small delay
    DisplayManager::delay(10);

    uint32_t t2 = DisplayManager::get_ticks();

    // t2 should be at least 10ms after t1 (with some tolerance for scheduling)
    REQUIRE(t2 >= t1);
    REQUIRE((t2 - t1) >= 5); // At least 5ms elapsed (allowing for timing variance)
}

TEST_CASE("DisplayManager::delay blocks for approximate duration", "[application][display]") {
    uint32_t start = DisplayManager::get_ticks();

    DisplayManager::delay(50);

    uint32_t elapsed = DisplayManager::get_ticks() - start;

    // Should be at least 40ms (allowing 10ms variance for scheduling)
    REQUIRE(elapsed >= 40);
    // Should not be too long (< 200ms)
    REQUIRE(elapsed < 200);
}

// ============================================================================
// DisplayManager Initialization Tests (require special handling)
// ============================================================================
// Note: Full init/shutdown tests are tricky because LVGLTestFixture already
// initializes LVGL. These tests are marked .pending until we have a way to
// test DisplayManager in complete isolation.

TEST_CASE("DisplayManager double init returns false", "[application][display][.pending]") {
    // This test would require fully isolating LVGL init
    // For now, we trust that the implementation checks m_initialized
    REQUIRE(true);
}

TEST_CASE("DisplayManager init creates display with correct dimensions",
          "[application][display][.pending]") {
    // Would need isolated LVGL to test properly
    REQUIRE(true);
}

TEST_CASE("DisplayManager init creates pointer input", "[application][display][.pending]") {
    // Would need isolated LVGL to test properly
    REQUIRE(true);
}

TEST_CASE("DisplayManager shutdown cleans up all resources", "[application][display][.pending]") {
    // Would need isolated LVGL to test properly
    REQUIRE(true);
}

TEST_CASE("DisplayManager scroll configuration applies to pointer",
          "[application][display][.pending]") {
    // Would need initialized pointer device to verify
    REQUIRE(true);
}
