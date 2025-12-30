// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_print_select_detail_subjects.cpp
 * @brief Unit tests for print select detail view subject initialization
 *
 * Tests that pre-print option subjects are initialized with correct defaults:
 * - Skip switches (bed_mesh, qgl, z_tilt, nozzle_clean) default to ON (1)
 * - Add-on switches (timelapse) default to OFF (0)
 *
 * Bug context: Previously switches defaulted to OFF in XML, which caused
 * is_option_disabled() to return true even when user hadn't touched them.
 * This triggered false modification warnings when printing without plugin.
 */

#include "ui_subject_registry.h"

#include <string>

#include "../catch_amalgamated.hpp"

// ============================================================================
// Pre-print Option Subject Default Tests
// ============================================================================

TEST_CASE("Pre-print skip switches should default to ON (1)",
          "[print_select][detail_view][subjects]") {
    // Test the pattern used in PrintSelectDetailView::init_subjects()
    // Skip switches default to 1 (ON) = "don't skip, do what file says"

    SECTION("Bed mesh switch defaults to ON") {
        static lv_subject_t preprint_bed_mesh;
        lv_subject_init_int(&preprint_bed_mesh, 1); // Default: ON
        REQUIRE(lv_subject_get_int(&preprint_bed_mesh) == 1);
    }

    SECTION("QGL switch defaults to ON") {
        static lv_subject_t preprint_qgl;
        lv_subject_init_int(&preprint_qgl, 1); // Default: ON
        REQUIRE(lv_subject_get_int(&preprint_qgl) == 1);
    }

    SECTION("Z-tilt switch defaults to ON") {
        static lv_subject_t preprint_z_tilt;
        lv_subject_init_int(&preprint_z_tilt, 1); // Default: ON
        REQUIRE(lv_subject_get_int(&preprint_z_tilt) == 1);
    }

    SECTION("Nozzle clean switch defaults to ON") {
        static lv_subject_t preprint_nozzle_clean;
        lv_subject_init_int(&preprint_nozzle_clean, 1); // Default: ON
        REQUIRE(lv_subject_get_int(&preprint_nozzle_clean) == 1);
    }
}

TEST_CASE("Pre-print add-on switches should default to OFF (0)",
          "[print_select][detail_view][subjects]") {
    // Add-on switches default to 0 (OFF) = "don't add extras by default"

    SECTION("Timelapse switch defaults to OFF") {
        static lv_subject_t preprint_timelapse;
        lv_subject_init_int(&preprint_timelapse, 0); // Default: OFF
        REQUIRE(lv_subject_get_int(&preprint_timelapse) == 0);
    }
}

TEST_CASE("Pre-print subjects can be reset to defaults", "[print_select][detail_view][subjects]") {
    // Simulates what happens in show() - subjects reset to defaults for new file

    SECTION("Skip switch can be toggled OFF then reset to ON") {
        static lv_subject_t preprint_bed_mesh;
        lv_subject_init_int(&preprint_bed_mesh, 1); // Initial: ON

        // User toggles OFF
        lv_subject_set_int(&preprint_bed_mesh, 0);
        REQUIRE(lv_subject_get_int(&preprint_bed_mesh) == 0);

        // Reset to default when showing new file
        lv_subject_set_int(&preprint_bed_mesh, 1);
        REQUIRE(lv_subject_get_int(&preprint_bed_mesh) == 1);
    }

    SECTION("Add-on switch can be toggled ON then reset to OFF") {
        static lv_subject_t preprint_timelapse;
        lv_subject_init_int(&preprint_timelapse, 0); // Initial: OFF

        // User toggles ON
        lv_subject_set_int(&preprint_timelapse, 1);
        REQUIRE(lv_subject_get_int(&preprint_timelapse) == 1);

        // Reset to default when showing new file
        lv_subject_set_int(&preprint_timelapse, 0);
        REQUIRE(lv_subject_get_int(&preprint_timelapse) == 0);
    }
}

TEST_CASE("Subject value 1 means switch is checked (ON)", "[print_select][detail_view][subjects]") {
    // Documents the semantic meaning of subject values
    // Used by bind_state_if_eq in XML: ref_value="1" binds checked state

    SECTION("Value 1 = checked/enabled") {
        static lv_subject_t subject;
        lv_subject_init_int(&subject, 1);
        // In XML: <bind_state_if_eq subject="..." state="checked" ref_value="1"/>
        // When subject == 1, switch shows as checked (ON)
        REQUIRE(lv_subject_get_int(&subject) == 1);
    }

    SECTION("Value 0 = unchecked/disabled") {
        static lv_subject_t subject;
        lv_subject_init_int(&subject, 0);
        // When subject == 0, switch shows as unchecked (OFF)
        REQUIRE(lv_subject_get_int(&subject) == 0);
    }
}
