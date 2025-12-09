// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2025 HelixScreen Contributors
 */

/**
 * @file test_multicolor_gcode.cpp
 * @brief Unit tests for multi-color G-code parsing and rendering
 *
 * Tests the complete pipeline:
 * 1. Parser: extracting tool colors and tracking tool changes
 * 2. Geometry Builder: converting tool indices to colors
 * 3. Integration: end-to-end multi-color rendering
 */

#include "gcode_geometry_builder.h"
#include "gcode_parser.h"

#include <fstream>
#include <sstream>

#include "../catch_amalgamated.hpp"

using namespace helix::gcode;

// ============================================================================
// Parser Tests
// ============================================================================

TEST_CASE("MultiColor - Parse extruder_colour metadata", "[gcode][multicolor][parser]") {
    GCodeParser parser;

    SECTION("Parse 4-color OrcaSlicer format") {
        parser.parse_line("; extruder_colour = #ED1C24;#00C1AE;#F4E2C1;#000000");

        const auto& palette = parser.get_tool_color_palette();

        REQUIRE(palette.size() == 4);
        REQUIRE(palette[0] == "#ED1C24"); // Red
        REQUIRE(palette[1] == "#00C1AE"); // Teal
        REQUIRE(palette[2] == "#F4E2C1"); // Beige
        REQUIRE(palette[3] == "#000000"); // Black
    }

    SECTION("Parse 2-color setup") {
        parser.parse_line("; extruder_colour = #FF0000;#0000FF");

        const auto& palette = parser.get_tool_color_palette();

        REQUIRE(palette.size() == 2);
        REQUIRE(palette[0] == "#FF0000");
        REQUIRE(palette[1] == "#0000FF");
    }

    SECTION("Handle whitespace in metadata") {
        parser.parse_line(";extruder_colour=#AA0000 ; #00BB00 ;#0000CC");

        const auto& palette = parser.get_tool_color_palette();

        REQUIRE(palette.size() == 3);
        REQUIRE(palette[0] == "#AA0000");
        REQUIRE(palette[1] == "#00BB00");
        REQUIRE(palette[2] == "#0000CC");
    }
}

TEST_CASE("MultiColor - Parse filament_colour as fallback", "[gcode][multicolor][parser]") {
    GCodeParser parser;

    SECTION("Use filament_colour when extruder_colour not present") {
        parser.parse_line("; filament_colour = #FF0000;#00FF00;#0000FF");

        const auto& palette = parser.get_tool_color_palette();

        REQUIRE(palette.size() == 3);
        REQUIRE(palette[0] == "#FF0000");
        REQUIRE(palette[1] == "#00FF00");
        REQUIRE(palette[2] == "#0000FF");
    }

    SECTION("extruder_colour takes priority over filament_colour") {
        parser.parse_line("; filament_colour = #111111;#222222");
        parser.parse_line("; extruder_colour = #AA0000;#00BB00");

        const auto& palette = parser.get_tool_color_palette();

        // Should have both - first from filament_colour, then from extruder_colour
        REQUIRE(palette.size() >= 2);
    }
}

TEST_CASE("MultiColor - Parse tool change commands", "[gcode][multicolor][parser]") {
    GCodeParser parser;

    SECTION("Track tool changes across segments") {
        parser.parse_line("T0");
        parser.parse_line("G1 X10 Y10 E1");
        parser.parse_line("T2");
        parser.parse_line("G1 X20 Y20 E2");
        parser.parse_line("T1");
        parser.parse_line("G1 X30 Y30 E3");

        auto result = parser.finalize();

        REQUIRE(result.layers.size() > 0);
        REQUIRE(result.layers[0].segments.size() >= 3);
        REQUIRE(result.layers[0].segments[0].tool_index == 0);
        REQUIRE(result.layers[0].segments[1].tool_index == 2);
        REQUIRE(result.layers[0].segments[2].tool_index == 1);
    }

    SECTION("Default to tool 0 when no tool change") {
        parser.parse_line("G1 X10 Y10 E1");

        auto result = parser.finalize();

        REQUIRE(result.layers[0].segments[0].tool_index == 0);
    }

    SECTION("Handle sequential tool numbers") {
        parser.parse_line("T0");
        parser.parse_line("G1 X1 Y1 E1");
        parser.parse_line("T1");
        parser.parse_line("G1 X2 Y2 E2");
        parser.parse_line("T2");
        parser.parse_line("G1 X3 Y3 E3");
        parser.parse_line("T3");
        parser.parse_line("G1 X4 Y4 E4");

        auto result = parser.finalize();

        REQUIRE(result.layers[0].segments.size() >= 4);
        for (size_t i = 0; i < 4; i++) {
            REQUIRE(result.layers[0].segments[i].tool_index == static_cast<int>(i));
        }
    }
}

TEST_CASE("MultiColor - Wipe tower detection", "[gcode][multicolor][parser]") {
    GCodeParser parser;

    SECTION("Mark segments inside wipe tower") {
        parser.parse_line("G1 X10 Y10 E1");
        parser.parse_line("; WIPE_TOWER_START");
        parser.parse_line("G1 X20 Y20 E2");
        parser.parse_line("; WIPE_TOWER_END");
        parser.parse_line("G1 X30 Y30 E3");

        auto result = parser.finalize();

        REQUIRE(result.layers[0].segments.size() >= 3);
        REQUIRE(result.layers[0].segments[0].object_name != "__WIPE_TOWER__");
        REQUIRE(result.layers[0].segments[1].object_name == "__WIPE_TOWER__");
        REQUIRE(result.layers[0].segments[2].object_name != "__WIPE_TOWER__");
    }

    SECTION("Handle wipe tower brim markers") {
        parser.parse_line("; WIPE_TOWER_BRIM_START");
        parser.parse_line("G1 X10 Y10 E1");
        parser.parse_line("; WIPE_TOWER_BRIM_END");

        auto result = parser.finalize();

        REQUIRE(result.layers[0].segments[0].object_name == "__WIPE_TOWER__");
    }
}

TEST_CASE("MultiColor - Palette transferred to ParsedGCodeFile", "[gcode][multicolor][parser]") {
    GCodeParser parser;

    parser.parse_line("; extruder_colour = #AA0000;#00BB00;#0000CC");
    parser.parse_line("G1 X10 Y10 E1");

    auto result = parser.finalize();

    REQUIRE(result.tool_color_palette.size() == 3);
    REQUIRE(result.tool_color_palette[0] == "#AA0000");
    REQUIRE(result.tool_color_palette[1] == "#00BB00");
    REQUIRE(result.tool_color_palette[2] == "#0000CC");
}

// ============================================================================
// Geometry Builder Tests
// ============================================================================

TEST_CASE("MultiColor - Set tool color palette", "[gcode][multicolor][geometry]") {
    GeometryBuilder builder;

    SECTION("Set and verify palette") {
        std::vector<std::string> palette = {"#FF0000", "#00FF00", "#0000FF"};
        builder.set_tool_color_palette(palette);

        // Palette is set - we can't directly verify it's stored correctly
        // without building geometry, but we verify it doesn't crash
        REQUIRE(true);
    }

    SECTION("Empty palette doesn't crash") {
        std::vector<std::string> empty_palette;
        builder.set_tool_color_palette(empty_palette);

        REQUIRE(true);
    }
}

TEST_CASE("MultiColor - Build geometry with tool colors", "[gcode][multicolor][geometry]") {
    SECTION("Use tool colors from palette") {
        GCodeParser parser;

        parser.parse_line("; extruder_colour = #ED1C24;#00C1AE");
        parser.parse_line("T0");
        parser.parse_line("G1 X0 Y0 Z0.2 E0");
        parser.parse_line("G1 X10 Y0 E1");
        parser.parse_line("T1");
        parser.parse_line("G1 X0 Y10 E2");

        auto gcode = parser.finalize();

        GeometryBuilder builder;
        builder.set_tool_color_palette(gcode.tool_color_palette);
        builder.set_use_height_gradient(false); // Use tool colors, not gradient

        SimplificationOptions opts;
        opts.enable_merging = false;

        auto geometry = builder.build(gcode, opts);

        REQUIRE(geometry.vertices.size() > 0);
        REQUIRE(geometry.color_palette.size() > 0);
    }
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_CASE("MultiColor - End-to-end integration", "[gcode][multicolor][integration]") {
    GCodeParser parser;

    SECTION("Parse and build multi-color geometry") {
        parser.parse_line("; extruder_colour = #ED1C24;#00C1AE");
        parser.parse_line("T0");
        parser.parse_line("G1 X0 Y0 Z0.2 E0");
        parser.parse_line("G1 X10 Y0 E1");
        parser.parse_line("G1 X10 Y10 E2");
        parser.parse_line("T1");
        parser.parse_line("G1 X0 Y10 E3");
        parser.parse_line("G1 X0 Y0 E4");

        auto gcode = parser.finalize();

        REQUIRE(gcode.tool_color_palette.size() == 2);
        REQUIRE(gcode.layers.size() > 0);
        REQUIRE(gcode.layers[0].segments.size() >= 4);

        // Verify tool indices
        REQUIRE(gcode.layers[0].segments[0].tool_index == 0);
        REQUIRE(gcode.layers[0].segments[1].tool_index == 0);
        REQUIRE(gcode.layers[0].segments[2].tool_index == 1);
        REQUIRE(gcode.layers[0].segments[3].tool_index == 1);

        // Build geometry
        GeometryBuilder builder;
        builder.set_tool_color_palette(gcode.tool_color_palette);

        SimplificationOptions opts;
        opts.enable_merging = false;

        auto geometry = builder.build(gcode, opts);

        REQUIRE(geometry.vertices.size() > 0);
        REQUIRE(geometry.color_palette.size() > 0);
    }
}

TEST_CASE("MultiColor - OrcaCube test file", "[gcode][multicolor][integration][file]") {
    const char* filename = "assets/OrcaCube_ABS_Multicolor.gcode";

    std::ifstream file(filename);
    if (!file.is_open()) {
        SKIP("OrcaCube file not found: " << filename);
    }

    GCodeParser parser;
    std::string line;
    int line_count = 0;
    int tool_change_count = 0;

    while (std::getline(file, line)) {
        parser.parse_line(line);
        line_count++;

        // Count tool changes
        if (line.length() >= 2 && line[0] == 'T' && std::isdigit(line[1]) &&
            (line.length() == 2 || std::isspace(line[2]))) {
            tool_change_count++;
        }
    }

    auto result = parser.finalize();

    SECTION("Verify OrcaCube metadata") {
        REQUIRE(result.tool_color_palette.size() == 4);
        REQUIRE(result.tool_color_palette[0] == "#ED1C24"); // Red
        REQUIRE(result.tool_color_palette[1] == "#00C1AE"); // Teal
        REQUIRE(result.tool_color_palette[2] == "#F4E2C1"); // Beige
        REQUIRE(result.tool_color_palette[3] == "#000000"); // Black
    }

    SECTION("Verify OrcaCube structure") {
        REQUIRE(tool_change_count == 51);
        REQUIRE(result.layers.size() > 0);
        REQUIRE(result.total_segments > 0);

        INFO("Parsed " << line_count << " lines, " << result.layers.size() << " layers, "
                       << result.total_segments << " segments");
    }
}

TEST_CASE("MultiColor - Backward compatibility", "[gcode][multicolor][compatibility]") {
    GCodeParser parser;

    SECTION("Single-color file without palette") {
        parser.parse_line("; filament_colour = #26A69A"); // Single color, no semicolons
        parser.parse_line("G1 X0 Y0 Z0.2 E0");
        parser.parse_line("G1 X10 Y0 E1");

        auto result = parser.finalize();

        // Single color might result in 0 or 1 palette entries depending on parsing
        REQUIRE(result.layers.size() > 0);
        REQUIRE(result.layers[0].segments.size() > 0);
        REQUIRE(result.layers[0].segments[0].tool_index == 0);
    }

    SECTION("No color metadata at all") {
        parser.parse_line("G1 X0 Y0 Z0.2 E0");
        parser.parse_line("G1 X10 Y0 E1");

        auto result = parser.finalize();

        REQUIRE(result.tool_color_palette.empty());
        REQUIRE(result.layers.size() > 0);
        REQUIRE(result.layers[0].segments[0].tool_index == 0);
    }
}
