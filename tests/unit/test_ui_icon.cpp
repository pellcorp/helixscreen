// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_ui_icon.cpp
 * @brief Unit tests for ui_icon.cpp - Icon widget with size, variant, and custom color support
 *
 * Tests cover:
 * - Size parsing (xs/sm/md/lg/xl) with valid and invalid values
 * - Variant parsing (primary/secondary/accent/disabled/success/warning/error/none)
 * - Public API functions (set_source, set_size, set_variant, set_color)
 * - Error handling (NULL pointers, invalid strings)
 *
 * Note: The implementation uses:
 * - IconSize enum (XS, SM, MD, LG, XL) - not a struct
 * - IconVariant enum (NONE, PRIMARY, SECONDARY, ACCENT, DISABLED, SUCCESS, WARNING, ERROR)
 * - Static internal functions (parse_size, parse_variant, apply_size, apply_variant)
 * - Public API uses the internal enums internally
 */

#include "../../include/ui_icon.h"
#include "../../include/ui_icon_codepoints.h"

#include <spdlog/spdlog.h>

#include "../catch_amalgamated.hpp"

// Test fixture for icon tests - manages spdlog level
class IconTest {
  public:
    IconTest() {
        spdlog::set_level(spdlog::level::debug);
    }
    ~IconTest() {
        spdlog::set_level(spdlog::level::warn);
    }
};

// ============================================================================
// Public API Tests - NULL pointer handling
// ============================================================================

TEST_CASE("ui_icon_set_source handles NULL icon", "[ui_icon][api][error]") {
    IconTest fixture;

    // Should log error and return without crashing
    REQUIRE_NOTHROW(ui_icon_set_source(nullptr, "home"));
}

TEST_CASE("ui_icon_set_source handles NULL icon_name", "[ui_icon][api][error]") {
    IconTest fixture;

    // Should log error and return without crashing
    // Note: Using dummy pointer - function should check for NULL before dereferencing
    REQUIRE_NOTHROW(ui_icon_set_source(reinterpret_cast<lv_obj_t*>(0x1), nullptr));
}

TEST_CASE("ui_icon_set_size handles NULL icon", "[ui_icon][api][error]") {
    IconTest fixture;

    REQUIRE_NOTHROW(ui_icon_set_size(nullptr, "md"));
}

TEST_CASE("ui_icon_set_size handles NULL size_str", "[ui_icon][api][error]") {
    IconTest fixture;

    REQUIRE_NOTHROW(ui_icon_set_size(reinterpret_cast<lv_obj_t*>(0x1), nullptr));
}

TEST_CASE("ui_icon_set_variant handles NULL icon", "[ui_icon][api][error]") {
    IconTest fixture;

    REQUIRE_NOTHROW(ui_icon_set_variant(nullptr, "primary"));
}

TEST_CASE("ui_icon_set_variant handles NULL variant_str", "[ui_icon][api][error]") {
    IconTest fixture;

    REQUIRE_NOTHROW(ui_icon_set_variant(reinterpret_cast<lv_obj_t*>(0x1), nullptr));
}

TEST_CASE("ui_icon_set_color handles NULL icon", "[ui_icon][api][error]") {
    IconTest fixture;

    lv_color_t color = lv_color_hex(0xFF0000);
    REQUIRE_NOTHROW(ui_icon_set_color(nullptr, color, LV_OPA_COVER));
}

// ============================================================================
// Icon Codepoint Lookup
// ============================================================================

TEST_CASE("Icon codepoint lookup returns valid codepoints", "[ui_icon][codepoint]") {
    IconTest fixture;

    // Test common icons
    const char* home = ui_icon::lookup_codepoint("home");
    REQUIRE(home != nullptr);

    const char* wifi = ui_icon::lookup_codepoint("wifi");
    REQUIRE(wifi != nullptr);

    const char* settings = ui_icon::lookup_codepoint("cog");
    REQUIRE(settings != nullptr);
}

TEST_CASE("Icon codepoint lookup returns nullptr for unknown icons", "[ui_icon][codepoint]") {
    IconTest fixture;

    const char* unknown = ui_icon::lookup_codepoint("nonexistent_icon_xyz");
    REQUIRE(unknown == nullptr);
}

TEST_CASE("Icon codepoint lookup handles NULL", "[ui_icon][codepoint][error]") {
    IconTest fixture;

    const char* result = ui_icon::lookup_codepoint(nullptr);
    REQUIRE(result == nullptr);
}

TEST_CASE("Icon codepoint lookup handles empty string", "[ui_icon][codepoint][error]") {
    IconTest fixture;

    const char* result = ui_icon::lookup_codepoint("");
    REQUIRE(result == nullptr);
}

// ============================================================================
// Legacy Prefix Stripping
// ============================================================================

TEST_CASE("strip_legacy_prefix removes mat_ prefix", "[ui_icon][legacy]") {
    IconTest fixture;

    const char* result = ui_icon::strip_legacy_prefix("mat_home");
    REQUIRE(strcmp(result, "home") == 0);
}

TEST_CASE("strip_legacy_prefix does NOT strip _img suffix without mat_ prefix",
          "[ui_icon][legacy]") {
    IconTest fixture;

    // The implementation ONLY handles names starting with "mat_"
    // A plain "_img" suffix without "mat_" prefix is NOT stripped
    const char* result = ui_icon::strip_legacy_prefix("home_img");
    REQUIRE(strcmp(result, "home_img") == 0); // Returns original, unchanged
}

TEST_CASE("strip_legacy_prefix removes both prefix and suffix", "[ui_icon][legacy]") {
    IconTest fixture;

    const char* result = ui_icon::strip_legacy_prefix("mat_wifi_img");
    REQUIRE(strcmp(result, "wifi") == 0);
}

TEST_CASE("strip_legacy_prefix returns original if no prefix/suffix", "[ui_icon][legacy]") {
    IconTest fixture;

    const char* result = ui_icon::strip_legacy_prefix("wifi");
    REQUIRE(strcmp(result, "wifi") == 0);
}

TEST_CASE("strip_legacy_prefix handles NULL", "[ui_icon][legacy][error]") {
    IconTest fixture;

    const char* result = ui_icon::strip_legacy_prefix(nullptr);
    REQUIRE(result == nullptr);
}

TEST_CASE("strip_legacy_prefix handles empty string", "[ui_icon][legacy][error]") {
    IconTest fixture;

    const char* result = ui_icon::strip_legacy_prefix("");
    REQUIRE(result != nullptr);
    REQUIRE(strlen(result) == 0);
}
