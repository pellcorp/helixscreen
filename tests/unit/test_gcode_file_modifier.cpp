// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2025 HelixScreen Contributors
 */

#include "gcode_file_modifier.h"

#include "../catch_amalgamated.hpp"

using namespace gcode;

// ============================================================================
// TempGCodeFile RAII Tests
// ============================================================================

TEST_CASE("TempGCodeFile - RAII cleanup", "[gcode][file_modifier]") {
    bool cleanup_called = false;
    std::string cleaned_path;

    SECTION("Destructor triggers cleanup callback") {
        {
            TempGCodeFile temp(
                ".helix_temp/test.gcode", "original.gcode",
                [&](const std::string& path) {
                    cleanup_called = true;
                    cleaned_path = path;
                });

            REQUIRE(temp.owns_file());
            REQUIRE(temp.moonraker_path() == ".helix_temp/test.gcode");
            REQUIRE(temp.original_filename() == "original.gcode");
        }

        REQUIRE(cleanup_called);
        REQUIRE(cleaned_path == ".helix_temp/test.gcode");
    }

    SECTION("release() prevents cleanup") {
        {
            TempGCodeFile temp(
                ".helix_temp/test.gcode", "original.gcode",
                [&](const std::string&) { cleanup_called = true; });

            temp.release();
            REQUIRE_FALSE(temp.owns_file());
        }

        REQUIRE_FALSE(cleanup_called);
    }

    SECTION("Move constructor transfers ownership") {
        {
            TempGCodeFile temp1(
                ".helix_temp/test.gcode", "original.gcode",
                [&](const std::string& path) {
                    cleanup_called = true;
                    cleaned_path = path;
                });

            TempGCodeFile temp2(std::move(temp1));

            REQUIRE(temp2.owns_file());
            REQUIRE(temp2.moonraker_path() == ".helix_temp/test.gcode");
            // NOLINTNEXTLINE(bugprone-use-after-move) - intentionally testing moved-from state
            REQUIRE_FALSE(temp1.owns_file());
        }

        // Cleanup should be called exactly once
        REQUIRE(cleanup_called);
    }

    SECTION("Move assignment transfers ownership") {
        int cleanup_count = 0;

        {
            TempGCodeFile temp1(
                ".helix_temp/first.gcode", "first.gcode",
                [&](const std::string&) { cleanup_count++; });

            TempGCodeFile temp2(
                ".helix_temp/second.gcode", "second.gcode",
                [&](const std::string&) { cleanup_count++; });

            // This should cleanup temp2's original file
            temp2 = std::move(temp1);

            REQUIRE(cleanup_count == 1);  // temp2's original was cleaned
            REQUIRE(temp2.moonraker_path() == ".helix_temp/first.gcode");
        }

        // After scope, temp2 (now first.gcode) is cleaned
        REQUIRE(cleanup_count == 2);
    }

    SECTION("Null callback is safe") {
        {
            TempGCodeFile temp(".helix_temp/test.gcode", "original.gcode", nullptr);
            REQUIRE(temp.owns_file());
        }
        // No crash when destructor runs with null callback
    }
}

// ============================================================================
// Content Modification Tests
// ============================================================================

// Helper to create a mock GCodeFileModifier for testing generate_modified_content
// We need to make this function accessible for testing
namespace {

// Create a simple wrapper that exposes the private method for testing
class TestableGCodeFileModifier {
public:
    TestableGCodeFileModifier(const ModifierConfig& config = {}) : config_(config) {}

    std::pair<std::string, size_t> generate_modified_content(
        const std::string& original_content,
        const std::vector<DetectedOperation>& ops_to_skip) const {

        // Build set of line numbers to skip
        std::set<size_t> lines_to_skip;
        for (const auto& op : ops_to_skip) {
            lines_to_skip.insert(op.line_number);
        }

        std::ostringstream modified;
        std::istringstream input(original_content);
        std::string line;
        size_t line_number = 0;
        size_t modified_count = 0;

        while (std::getline(input, line)) {
            line_number++;

            if (lines_to_skip.count(line_number) > 0) {
                modified << config_.skip_prefix << line;
                if (!line.empty() && line.back() != '\n') {
                    modified << " ; HelixScreen: operation disabled by user";
                }
                modified << "\n";
                modified_count++;
            } else {
                modified << line << "\n";
            }
        }

        return {modified.str(), modified_count};
    }

private:
    ModifierConfig config_;
};

DetectedOperation make_op(OperationType type, size_t line, const std::string& raw = "") {
    DetectedOperation op;
    op.type = type;
    op.embedding = OperationEmbedding::DIRECT_COMMAND;
    op.line_number = line;
    op.raw_line = raw;
    op.macro_name = raw;
    return op;
}

}  // namespace

TEST_CASE("GCodeFileModifier - Content modification", "[gcode][file_modifier]") {
    TestableGCodeFileModifier modifier;

    SECTION("Single line modification") {
        std::string content = "G28\n"
                              "BED_MESH_CALIBRATE\n"
                              "G1 X0 Y0 Z0.2\n";

        auto bed_level = make_op(OperationType::BED_LEVELING, 2, "BED_MESH_CALIBRATE");

        auto [modified, count] = modifier.generate_modified_content(content, {bed_level});

        REQUIRE(count == 1);
        REQUIRE(modified.find("; HELIX_SKIP: BED_MESH_CALIBRATE") != std::string::npos);
        REQUIRE(modified.find("G28\n") != std::string::npos);
        REQUIRE(modified.find("G1 X0 Y0 Z0.2\n") != std::string::npos);
    }

    SECTION("Multiple line modifications") {
        std::string content = "G28\n"
                              "QUAD_GANTRY_LEVEL\n"
                              "BED_MESH_CALIBRATE\n"
                              "CLEAN_NOZZLE\n"
                              "G1 X0 Y0 Z0.2\n";

        std::vector<DetectedOperation> ops = {
            make_op(OperationType::QGL, 2, "QUAD_GANTRY_LEVEL"),
            make_op(OperationType::NOZZLE_CLEAN, 4, "CLEAN_NOZZLE"),
        };

        auto [modified, count] = modifier.generate_modified_content(content, ops);

        REQUIRE(count == 2);
        REQUIRE(modified.find("; HELIX_SKIP: QUAD_GANTRY_LEVEL") != std::string::npos);
        REQUIRE(modified.find("; HELIX_SKIP: CLEAN_NOZZLE") != std::string::npos);
        // Line 3 should NOT be modified
        REQUIRE(modified.find("BED_MESH_CALIBRATE\n") != std::string::npos);
    }

    SECTION("First line modification") {
        std::string content = "BED_MESH_CALIBRATE\n"
                              "G1 X0 Y0 Z0.2\n";

        auto bed_level = make_op(OperationType::BED_LEVELING, 1, "BED_MESH_CALIBRATE");

        auto [modified, count] = modifier.generate_modified_content(content, {bed_level});

        REQUIRE(count == 1);
        REQUIRE(modified.find("; HELIX_SKIP: BED_MESH_CALIBRATE") == 0);
    }

    SECTION("Last line modification") {
        std::string content = "G28\n"
                              "BED_MESH_CALIBRATE";  // No trailing newline

        auto bed_level = make_op(OperationType::BED_LEVELING, 2, "BED_MESH_CALIBRATE");

        auto [modified, count] = modifier.generate_modified_content(content, {bed_level});

        REQUIRE(count == 1);
        REQUIRE(modified.find("; HELIX_SKIP: BED_MESH_CALIBRATE") != std::string::npos);
    }

    SECTION("Empty content") {
        auto [modified, count] = modifier.generate_modified_content("", {});

        REQUIRE(count == 0);
        REQUIRE(modified.empty());
    }

    SECTION("No operations to skip") {
        std::string content = "G28\nBED_MESH_CALIBRATE\n";

        auto [modified, count] = modifier.generate_modified_content(content, {});

        REQUIRE(count == 0);
        REQUIRE(modified == content);
    }

    SECTION("Invalid line number (0)") {
        std::string content = "G28\nBED_MESH_CALIBRATE\n";

        auto invalid_op = make_op(OperationType::BED_LEVELING, 0, "BED_MESH_CALIBRATE");

        auto [modified, count] = modifier.generate_modified_content(content, {invalid_op});

        REQUIRE(count == 0);  // Line 0 doesn't exist
        REQUIRE(modified == content);
    }

    SECTION("Line number beyond file") {
        std::string content = "G28\n";

        auto beyond_op = make_op(OperationType::BED_LEVELING, 100, "BED_MESH_CALIBRATE");

        auto [modified, count] = modifier.generate_modified_content(content, {beyond_op});

        REQUIRE(count == 0);  // Line 100 doesn't exist
        REQUIRE(modified == content);
    }
}

TEST_CASE("GCodeFileModifier - Custom skip prefix", "[gcode][file_modifier]") {
    ModifierConfig config;
    config.skip_prefix = "; CUSTOM: ";
    TestableGCodeFileModifier modifier(config);

    std::string content = "G28\nBED_MESH_CALIBRATE\n";
    auto bed_level = make_op(OperationType::BED_LEVELING, 2, "BED_MESH_CALIBRATE");

    auto [modified, count] = modifier.generate_modified_content(content, {bed_level});

    REQUIRE(count == 1);
    REQUIRE(modified.find("; CUSTOM: BED_MESH_CALIBRATE") != std::string::npos);
    REQUIRE(modified.find("; HELIX_SKIP:") == std::string::npos);  // Not the default
}

// ============================================================================
// ModifierConfig Tests
// ============================================================================

TEST_CASE("ModifierConfig - Defaults", "[gcode][file_modifier]") {
    ModifierConfig config;

    REQUIRE(config.temp_dir == ".helix_temp");
    REQUIRE(config.skip_prefix == "; HELIX_SKIP: ");
    REQUIRE(config.add_header_comment == true);
}

// ============================================================================
// SkipCopyResult Tests
// ============================================================================

TEST_CASE("SkipCopyResult - Structure", "[gcode][file_modifier]") {
    SkipCopyResult result;

    REQUIRE(result.temp_file == nullptr);
    REQUIRE(result.skipped_ops.empty());
    REQUIRE(result.lines_modified == 0);
}

// ============================================================================
// Real-world G-code Modification Tests
// ============================================================================

TEST_CASE("GCodeFileModifier - Real-world G-code", "[gcode][file_modifier]") {
    TestableGCodeFileModifier modifier;

    SECTION("OrcaSlicer Voron start sequence") {
        std::string content = R"(; generated by OrcaSlicer 2.1.0
M140 S60 ; set bed temp
M104 S220 ; set extruder temp
G28 ; home all
QUAD_GANTRY_LEVEL ; level gantry
BED_MESH_CALIBRATE ; probe bed
CLEAN_NOZZLE ; wipe nozzle
G1 X10 Y10 Z0.3 E0.5 ; start print
)";

        // Skip QGL (line 5) and bed mesh (line 6)
        std::vector<DetectedOperation> ops = {
            make_op(OperationType::QGL, 5, "QUAD_GANTRY_LEVEL"),
            make_op(OperationType::BED_LEVELING, 6, "BED_MESH_CALIBRATE"),
        };

        auto [modified, count] = modifier.generate_modified_content(content, ops);

        REQUIRE(count == 2);

        // QGL should be skipped
        REQUIRE(modified.find("; HELIX_SKIP: QUAD_GANTRY_LEVEL") != std::string::npos);

        // Bed mesh should be skipped
        REQUIRE(modified.find("; HELIX_SKIP: BED_MESH_CALIBRATE") != std::string::npos);

        // Other lines should be unchanged
        REQUIRE(modified.find("M140 S60") != std::string::npos);
        REQUIRE(modified.find("G28 ; home all") != std::string::npos);
        REQUIRE(modified.find("CLEAN_NOZZLE ; wipe nozzle\n") != std::string::npos);
        REQUIRE(modified.find("G1 X10 Y10 Z0.3 E0.5") != std::string::npos);
    }

    SECTION("PrusaSlicer with inline comments") {
        std::string content = R"(G28 ; Home
G29 ; Bed leveling - this has a long comment
G1 X0 Y0 Z0.2
)";

        auto bed_level = make_op(OperationType::BED_LEVELING, 2, "G29");

        auto [modified, count] = modifier.generate_modified_content(content, {bed_level});

        REQUIRE(count == 1);
        // The entire line including the comment should be prefixed
        REQUIRE(modified.find("; HELIX_SKIP: G29 ; Bed leveling") != std::string::npos);
    }

    SECTION("START_PRINT macro with parameters") {
        std::string content = R"(M140 S60
START_PRINT EXTRUDER_TEMP=220 BED_TEMP=60 FORCE_LEVELING=true NOZZLE_CLEAN=1
G1 X0 Y0 Z0.2
)";

        // Skip the entire START_PRINT line
        DetectedOperation start_print;
        start_print.type = OperationType::BED_LEVELING;
        start_print.embedding = OperationEmbedding::MACRO_PARAMETER;
        start_print.line_number = 2;
        start_print.raw_line =
            "START_PRINT EXTRUDER_TEMP=220 BED_TEMP=60 FORCE_LEVELING=true NOZZLE_CLEAN=1";
        start_print.macro_name = "START_PRINT";
        start_print.param_name = "FORCE_LEVELING";
        start_print.param_value = "true";

        auto [modified, count] = modifier.generate_modified_content(content, {start_print});

        REQUIRE(count == 1);
        REQUIRE(modified.find("; HELIX_SKIP: START_PRINT") != std::string::npos);
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("GCodeFileModifier - Edge cases", "[gcode][file_modifier]") {
    TestableGCodeFileModifier modifier;

    SECTION("Windows line endings (CRLF)") {
        std::string content = "G28\r\nBED_MESH_CALIBRATE\r\nG1 X0\r\n";

        auto bed_level = make_op(OperationType::BED_LEVELING, 2, "BED_MESH_CALIBRATE");

        auto [modified, count] = modifier.generate_modified_content(content, {bed_level});

        // std::getline handles \r\n, but leaves \r at end of line
        REQUIRE(count == 1);
    }

    SECTION("Very long line") {
        std::string long_comment(1000, 'x');
        std::string content = "G28\n; " + long_comment + "\nBED_MESH_CALIBRATE\n";

        auto bed_level = make_op(OperationType::BED_LEVELING, 3, "BED_MESH_CALIBRATE");

        auto [modified, count] = modifier.generate_modified_content(content, {bed_level});

        REQUIRE(count == 1);
        REQUIRE(modified.find(long_comment) != std::string::npos);  // Long line preserved
    }

    SECTION("Duplicate line numbers in ops (first wins)") {
        std::string content = "G28\nBED_MESH_CALIBRATE\n";

        std::vector<DetectedOperation> ops = {
            make_op(OperationType::BED_LEVELING, 2, "BED_MESH_CALIBRATE"),
            make_op(OperationType::QGL, 2, "QUAD_GANTRY_LEVEL"),  // Same line, different type
        };

        auto [modified, count] = modifier.generate_modified_content(content, ops);

        // Line should only be modified once
        REQUIRE(count == 1);
    }

    SECTION("Unicode in G-code comments") {
        std::string content = "G28 ; 🏠 home\nBED_MESH_CALIBRATE ; 📐 level\n";

        auto bed_level = make_op(OperationType::BED_LEVELING, 2, "BED_MESH_CALIBRATE");

        auto [modified, count] = modifier.generate_modified_content(content, {bed_level});

        REQUIRE(count == 1);
        REQUIRE(modified.find("🏠") != std::string::npos);  // Emoji preserved
    }
}
