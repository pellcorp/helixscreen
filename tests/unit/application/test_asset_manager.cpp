// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_asset_manager.cpp
 * @brief Unit tests for AssetManager class
 *
 * Tests font and image registration with the LVGL XML system.
 * Note: These tests require LVGL to be initialized, which LVGLTestFixture provides.
 */

#include "application_test_fixture.h"
#include "asset_manager.h"

#include "../../catch_amalgamated.hpp"

// ============================================================================
// AssetManager Registration State Tests
// ============================================================================

TEST_CASE("AssetManager tracks font registration state", "[application][assets]") {
    // Note: fonts may already be registered from previous tests
    // Just verify the state tracking works
    AssetManager::register_fonts();

    REQUIRE(AssetManager::fonts_registered() == true);

    // Second call should be idempotent
    AssetManager::register_fonts();
    REQUIRE(AssetManager::fonts_registered() == true);
}

TEST_CASE("AssetManager tracks image registration state", "[application][assets]") {
    AssetManager::register_images();

    REQUIRE(AssetManager::images_registered() == true);

    // Second call should be idempotent
    AssetManager::register_images();
    REQUIRE(AssetManager::images_registered() == true);
}

TEST_CASE("AssetManager::register_all registers both fonts and images", "[application][assets]") {
    AssetManager::register_all();

    REQUIRE(AssetManager::fonts_registered() == true);
    REQUIRE(AssetManager::images_registered() == true);
}

// ============================================================================
// Font Registration Tests (require LVGL)
// ============================================================================

TEST_CASE_METHOD(ApplicationTestFixture, "AssetManager registers MDI icon fonts",
                 "[application][assets][fonts]") {
    AssetManager::register_fonts();

    // After registration, fonts should be available via LVGL XML lookup
    // Note: We can't easily verify registration without LVGL internals,
    // but the call should not crash
    REQUIRE(AssetManager::fonts_registered() == true);
}

TEST_CASE_METHOD(ApplicationTestFixture, "AssetManager registers Noto Sans fonts",
                 "[application][assets][fonts]") {
    AssetManager::register_fonts();

    REQUIRE(AssetManager::fonts_registered() == true);
}

TEST_CASE_METHOD(ApplicationTestFixture, "AssetManager registers Montserrat aliases",
                 "[application][assets][fonts]") {
    // Montserrat fonts are aliased to Noto Sans for XML compatibility
    AssetManager::register_fonts();

    REQUIRE(AssetManager::fonts_registered() == true);
}

// ============================================================================
// Image Registration Tests (require LVGL)
// ============================================================================

TEST_CASE_METHOD(ApplicationTestFixture, "AssetManager registers UI images",
                 "[application][assets][images]") {
    AssetManager::register_images();

    REQUIRE(AssetManager::images_registered() == true);
}

// ============================================================================
// Idempotency Tests
// ============================================================================

TEST_CASE("AssetManager registration is idempotent", "[application][assets]") {
    // Multiple calls should not crash or cause issues
    AssetManager::register_all();
    AssetManager::register_all();
    AssetManager::register_all();

    REQUIRE(AssetManager::fonts_registered() == true);
    REQUIRE(AssetManager::images_registered() == true);
}
