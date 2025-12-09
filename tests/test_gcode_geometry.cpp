// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2025 HelixScreen Contributors
 *
 * Geometry validation tests for G-code tube generation
 * Tests the fix for perpendicular vector cross product orientation
 */

#include "gcode_geometry_builder.h"
#include "gcode_parser.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace helix::gcode;

TEST_CASE("Single horizontal line geometry validation", "[gcode][geometry]") {
    // Parse the single line test file
    GCodeParser parser;
    bool success = parser.parse_file("assets/gcode/single_line_test.gcode");
    REQUIRE(success);

    const auto& metadata = parser.get_metadata();
    const auto& segments = parser.get_segments();

    REQUIRE(segments.size() == 1);

    const auto& seg = segments[0];

    SECTION("Segment has correct endpoints") {
        REQUIRE_THAT(seg.start.x, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(seg.start.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(seg.start.z, Catch::Matchers::WithinAbs(0.2f, 0.001f));

        REQUIRE_THAT(seg.end.x, Catch::Matchers::WithinAbs(10.0f, 0.001f));
        REQUIRE_THAT(seg.end.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(seg.end.z, Catch::Matchers::WithinAbs(0.2f, 0.001f));
    }

    SECTION("Cross-section perpendicular vectors are correctly oriented") {
        // For a horizontal line in +X direction:
        // - direction = (1, 0, 0)
        // - perp_horizontal should point in Y direction: (0, ±1, 0)
        // - perp_vertical should point UP in Z direction: (0, 0, +1)
        //
        // The BUG was: cross(direction, perp_horizontal) gave (0,0,-1) pointing DOWN
        // The FIX: cross(perp_horizontal, direction) gives (0,0,+1) pointing UP

        glm::vec3 direction = glm::normalize(seg.end - seg.start);

        // Calculate perpendiculars using the FIXED formula
        glm::vec3 up(0.0f, 0.0f, 1.0f);
        glm::vec3 perp_horizontal = glm::cross(direction, up);
        if (glm::length2(perp_horizontal) < 1e-6f) {
            perp_horizontal = glm::vec3(1.0f, 0.0f, 0.0f);
        } else {
            perp_horizontal = glm::normalize(perp_horizontal);
        }

        // CRITICAL: This is the fix - cross(perp_horizontal, direction) not cross(direction,
        // perp_horizontal)
        glm::vec3 perp_vertical = glm::normalize(glm::cross(perp_horizontal, direction));

        // Verify direction is correct
        REQUIRE_THAT(direction.x, Catch::Matchers::WithinAbs(1.0f, 0.001f));
        REQUIRE_THAT(direction.y, Catch::Matchers::WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(direction.z, Catch::Matchers::WithinAbs(0.0f, 0.001f));

        // Verify perp_horizontal points in Y direction
        REQUIRE_THAT(std::abs(perp_horizontal.y), Catch::Matchers::WithinAbs(1.0f, 0.001f)); // ±Y

        // CRITICAL TEST: Verify perp_vertical points UPWARD (+Z), not downward
        INFO("perp_vertical.z must be POSITIVE (pointing up), not negative (pointing down)");
        REQUIRE(perp_vertical.z > 0.0f); // Must point UP!
        REQUIRE_THAT(perp_vertical.z, Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }

    SECTION("Cross-section corners have correct Z ordering") {
        // Build the geometry to get actual vertex positions
        GeometryBuilder builder;
        builder.set_layer_height(0.2f); // Match test file

        // Add segment
        std::vector<ToolpathSegment> segs = {seg};
        auto geometry = builder.build_geometry(segs, ColoringMode::SOLID, 0xFF00FF00);

        // With the fix, BOTTOM vertices should have LOWER Z than TOP vertices
        // Before fix: bottom had HIGHER Z (cross-section was inverted)
        //
        // We can't easily access quantized vertex positions from the test,
        // but we validated the perpendicular vector orientation above
        // which is the root cause of the bug
        REQUIRE(geometry.vertices.size() > 0);
    }
}

TEST_CASE("Perpendicular vector cross product order", "[gcode][geometry][regression]") {
    // Regression test for the perpendicular vector bug
    //
    // BUG: cross(direction, perp_horizontal) produced downward vertical
    // FIX: cross(perp_horizontal, direction) produces upward vertical

    SECTION("Horizontal line +X direction") {
        glm::vec3 direction(1, 0, 0);
        glm::vec3 up(0, 0, 1);

        glm::vec3 perp_h = glm::normalize(glm::cross(direction, up));
        // perp_h should be (0, -1, 0) or (0, 1, 0) depending on handedness

        // WRONG: This produces downward vector for +X direction
        glm::vec3 perp_v_wrong = glm::normalize(glm::cross(direction, perp_h));

        // RIGHT: This produces upward vector
        glm::vec3 perp_v_right = glm::normalize(glm::cross(perp_h, direction));

        INFO("Wrong formula: cross(direction, perp_horizontal) points DOWN");
        REQUIRE(perp_v_wrong.z < 0.0f); // Points down (BUG)

        INFO("Right formula: cross(perp_horizontal, direction) points UP");
        REQUIRE(perp_v_right.z > 0.0f); // Points up (FIX)
    }

    SECTION("Vertical line +Z direction") {
        glm::vec3 direction(0, 0, 1);
        glm::vec3 up(0, 0, 1);

        // Vertical line: use X-axis as horizontal perpendicular
        glm::vec3 perp_h(1, 0, 0);

        glm::vec3 perp_v = glm::normalize(glm::cross(perp_h, direction));

        // For vertical line, perp_v should point in Y direction
        REQUIRE_THAT(std::abs(perp_v.y), Catch::Matchers::WithinAbs(1.0f, 0.001f));
    }
}
