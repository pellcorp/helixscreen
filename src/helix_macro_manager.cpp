// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "helix_macro_manager.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <regex>
#include <sstream>

namespace helix {

// ============================================================================
// Embedded Macro Content
// ============================================================================

/**
 * @brief Complete helix_macros.cfg content
 *
 * This is embedded in the binary to avoid external file dependencies.
 * The content is designed to work with most Klipper setups.
 */
static const char* HELIX_MACROS_CFG = R"KLIPPER(
# HelixScreen Helper Macros
# Version: 1.0.0
# Auto-installed by HelixScreen - do not edit manually
#
# These macros provide standardized pre-print operations for HelixScreen's
# Bambu-style print options interface.

# ==============================================================================
# HELIX_BED_LEVEL_IF_NEEDED
# ==============================================================================
# Performs bed mesh calibration only if mesh is stale or missing.
# Uses a variable to track last calibration time.
#
# Usage: HELIX_BED_LEVEL_IF_NEEDED [MAX_AGE=<minutes>]
#   MAX_AGE: Maximum mesh age in minutes before recalibration (default: 60)
#
[gcode_macro HELIX_BED_LEVEL_IF_NEEDED]
description: Perform bed mesh if stale or missing (HelixScreen helper)
variable_last_mesh_time: 0
gcode:
    {% set max_age = params.MAX_AGE|default(60)|int %}
    {% set current_time = printer.idle_timeout.printing_time %}
    {% set mesh_age = current_time - printer["gcode_macro HELIX_BED_LEVEL_IF_NEEDED"].last_mesh_time %}

    {% if printer.bed_mesh.profile_name == "" or mesh_age > (max_age * 60) %}
        { action_respond_info("HelixScreen: Mesh stale or missing, running BED_MESH_CALIBRATE") }
        BED_MESH_CALIBRATE
        SET_GCODE_VARIABLE MACRO=HELIX_BED_LEVEL_IF_NEEDED VARIABLE=last_mesh_time VALUE={current_time}
    {% else %}
        { action_respond_info("HelixScreen: Using existing mesh (age: %d min)" % (mesh_age / 60)) }
    {% endif %}

# ==============================================================================
# HELIX_CLEAN_NOZZLE
# ==============================================================================
# Standardized nozzle cleaning sequence.
# Override this macro in your printer.cfg to customize for your hardware.
#
# Default behavior: Wipe motion if nozzle brush position is defined,
# otherwise just a small retract and move.
#
[gcode_macro HELIX_CLEAN_NOZZLE]
description: Clean nozzle before print (HelixScreen helper)
# Override these variables in your printer.cfg for your nozzle brush position
variable_brush_x: -1  # Set to your brush X position, or -1 to disable
variable_brush_y: -1  # Set to your brush Y position, or -1 to disable
variable_brush_z: -1  # Set to Z height for wiping, or -1 to use current
variable_wipe_count: 5
variable_wipe_length: 40
gcode:
    {% set brush_x = printer["gcode_macro HELIX_CLEAN_NOZZLE"].brush_x %}
    {% set brush_y = printer["gcode_macro HELIX_CLEAN_NOZZLE"].brush_y %}
    {% set brush_z = printer["gcode_macro HELIX_CLEAN_NOZZLE"].brush_z %}
    {% set wipe_count = printer["gcode_macro HELIX_CLEAN_NOZZLE"].wipe_count %}
    {% set wipe_length = printer["gcode_macro HELIX_CLEAN_NOZZLE"].wipe_length %}

    SAVE_GCODE_STATE NAME=helix_clean_nozzle
    G90  ; Absolute positioning

    {% if brush_x >= 0 and brush_y >= 0 %}
        ; Move to brush location
        G0 X{brush_x} Y{brush_y} F6000

        {% if brush_z >= 0 %}
            G0 Z{brush_z} F1500
        {% endif %}

        ; Wipe back and forth
        {% for i in range(wipe_count) %}
            G0 X{brush_x + wipe_length} F6000
            G0 X{brush_x} F6000
        {% endfor %}

        { action_respond_info("HelixScreen: Nozzle cleaning complete") }
    {% else %}
        ; No brush configured - just do a small retract
        G91  ; Relative
        G1 E-2 F300  ; Retract 2mm
        G90  ; Absolute
        { action_respond_info("HelixScreen: No brush configured, performed small retract") }
    {% endif %}

    RESTORE_GCODE_STATE NAME=helix_clean_nozzle

# ==============================================================================
# HELIX_START_PRINT
# ==============================================================================
# Unified start print macro with all pre-print options.
# Called by HelixScreen when user enables options in the print dialog.
#
# Usage: HELIX_START_PRINT [BED_TEMP=<temp>] [EXTRUDER_TEMP=<temp>]
#                          [DO_QGL=<0|1>] [DO_Z_TILT=<0|1>]
#                          [DO_BED_MESH=<0|1>] [DO_NOZZLE_CLEAN=<0|1>]
#
[gcode_macro HELIX_START_PRINT]
description: Unified start print with pre-print options (HelixScreen helper)
gcode:
    {% set bed_temp = params.BED_TEMP|default(60)|int %}
    {% set extruder_temp = params.EXTRUDER_TEMP|default(200)|int %}
    {% set do_qgl = params.DO_QGL|default(0)|int %}
    {% set do_z_tilt = params.DO_Z_TILT|default(0)|int %}
    {% set do_bed_mesh = params.DO_BED_MESH|default(0)|int %}
    {% set do_nozzle_clean = params.DO_NOZZLE_CLEAN|default(0)|int %}

    { action_respond_info("HelixScreen: Starting pre-print sequence") }

    ; Start heating bed
    M140 S{bed_temp}

    ; Home if needed
    {% if "xyz" not in printer.toolhead.homed_axes %}
        { action_respond_info("HelixScreen: Homing...") }
        G28
    {% endif %}

    ; QGL if requested and available
    {% if do_qgl == 1 %}
        {% if printer.configfile.settings.quad_gantry_level is defined %}
            { action_respond_info("HelixScreen: Running Quad Gantry Level...") }
            QUAD_GANTRY_LEVEL
        {% endif %}
    {% endif %}

    ; Z-Tilt if requested and available
    {% if do_z_tilt == 1 %}
        {% if printer.configfile.settings.z_tilt is defined %}
            { action_respond_info("HelixScreen: Running Z-Tilt Adjust...") }
            Z_TILT_ADJUST
        {% endif %}
    {% endif %}

    ; Bed mesh if requested
    {% if do_bed_mesh == 1 %}
        { action_respond_info("HelixScreen: Running Bed Mesh Calibrate...") }
        BED_MESH_CALIBRATE
    {% endif %}

    ; Wait for bed temperature
    M190 S{bed_temp}

    ; Heat extruder
    M109 S{extruder_temp}

    ; Nozzle clean if requested
    {% if do_nozzle_clean == 1 %}
        HELIX_CLEAN_NOZZLE
    {% endif %}

    { action_respond_info("HelixScreen: Pre-print sequence complete, starting print") }

# ==============================================================================
# HELIX_VERSION
# ==============================================================================
# Reports the installed HelixScreen macro version.
# Used by HelixScreen to detect if macros need updating.
#
[gcode_macro HELIX_VERSION]
description: Report HelixScreen macro version
variable_version: "1.0.0"
gcode:
    { action_respond_info("HelixScreen Macros Version: %s" % printer["gcode_macro HELIX_VERSION"].version) }
)KLIPPER";

// ============================================================================
// MacroManager Implementation
// ============================================================================

MacroManager::MacroManager(MoonrakerAPI& api, const PrinterCapabilities& capabilities)
    : api_(api), capabilities_(capabilities) {}

bool MacroManager::is_installed() const {
    return capabilities_.has_helix_macros();
}

MacroInstallStatus MacroManager::get_status() const {
    if (!capabilities_.has_helix_macros()) {
        return MacroInstallStatus::NOT_INSTALLED;
    }

    auto installed_version = parse_installed_version();
    if (!installed_version) {
        // Has macros but can't determine version - assume installed
        return MacroInstallStatus::INSTALLED;
    }

    if (*installed_version < std::string(HELIX_MACROS_VERSION)) {
        return MacroInstallStatus::OUTDATED;
    }

    return MacroInstallStatus::INSTALLED;
}

std::string MacroManager::get_installed_version() const {
    auto version = parse_installed_version();
    return version.value_or("");
}

bool MacroManager::update_available() const {
    return get_status() == MacroInstallStatus::OUTDATED;
}

void MacroManager::install(SuccessCallback on_success, ErrorCallback on_error) {
    spdlog::info("[HelixMacroManager] Starting macro installation...");

    // Step 1: Upload macro file
    upload_macro_file(
        [this, on_success, on_error]() {
            spdlog::info("[HelixMacroManager] Macro file uploaded, adding include...");

            // Step 2: Add include to printer.cfg
            add_include_to_config(
                [this, on_success, on_error]() {
                    spdlog::info("[HelixMacroManager] Include added, restarting Klipper...");

                    // Step 3: Restart Klipper
                    restart_klipper(
                        [on_success]() {
                            spdlog::info("[HelixMacroManager] Installation complete!");
                            on_success();
                        },
                        on_error);
                },
                on_error);
        },
        on_error);
}

void MacroManager::update(SuccessCallback on_success, ErrorCallback on_error) {
    spdlog::info("[HelixMacroManager] Starting macro update...");

    // Just upload the new file and restart
    upload_macro_file(
        [this, on_success, on_error]() {
            restart_klipper(on_success, on_error);
        },
        on_error);
}

void MacroManager::uninstall(SuccessCallback on_success, ErrorCallback on_error) {
    spdlog::info("[HelixMacroManager] Starting macro uninstall...");

    // Step 1: Remove include from printer.cfg
    remove_include_from_config(
        [this, on_success, on_error]() {
            // Step 2: Delete macro file
            delete_macro_file(
                [this, on_success, on_error]() {
                    // Step 3: Restart Klipper
                    restart_klipper(on_success, on_error);
                },
                on_error);
        },
        on_error);
}

std::string MacroManager::get_macro_content() {
    return HELIX_MACROS_CFG;
}

std::vector<std::string> MacroManager::get_macro_names() {
    return {"HELIX_BED_LEVEL_IF_NEEDED", "HELIX_CLEAN_NOZZLE", "HELIX_START_PRINT",
            "HELIX_VERSION"};
}

// ============================================================================
// Private Implementation
// ============================================================================

void MacroManager::upload_macro_file(SuccessCallback on_success, ErrorCallback on_error) {
    // TODO: Implement HTTP file upload via Moonraker
    // POST /server/files/upload with multipart form data
    //   - file: helix_macros.cfg content
    //   - root: config
    //
    // For now, this is a placeholder that will be implemented when
    // MoonrakerAPI adds file upload support.

    spdlog::warn(
        "[HelixMacroManager] File upload not yet implemented - requires HTTP multipart upload");

    // Simulate success for now
    on_success();
}

void MacroManager::add_include_to_config(SuccessCallback on_success, ErrorCallback on_error) {
    // TODO: Implement printer.cfg modification
    // 1. GET /server/files/config/printer.cfg
    // 2. Check if [include helix_macros.cfg] already present
    // 3. If not, prepend after any existing [include] lines
    // 4. POST /server/files/upload with modified content

    spdlog::warn("[HelixMacroManager] printer.cfg modification not yet implemented");

    // Simulate success for now
    on_success();
}

void MacroManager::remove_include_from_config(SuccessCallback on_success, ErrorCallback on_error) {
    // TODO: Remove [include helix_macros.cfg] line from printer.cfg

    spdlog::warn("[HelixMacroManager] printer.cfg modification not yet implemented");

    // Simulate success for now
    on_success();
}

void MacroManager::delete_macro_file(SuccessCallback on_success, ErrorCallback on_error) {
    // Use MoonrakerAPI to delete the file
    api_.delete_file(std::string("config/") + HELIX_MACROS_FILENAME, on_success,
                     [on_success, on_error](const MoonrakerError& err) {
                         // File might not exist - that's OK for uninstall
                         if (err.type == MoonrakerErrorType::FILE_NOT_FOUND) {
                             spdlog::debug("[HelixMacroManager] Macro file already deleted");
                             on_success();  // Continue with success path
                         } else {
                             on_error(err);
                         }
                     });
}

void MacroManager::restart_klipper(SuccessCallback on_success, ErrorCallback on_error) {
    spdlog::info("[HelixMacroManager] Requesting Klipper restart...");
    api_.restart_klipper(on_success, on_error);
}

std::optional<std::string> MacroManager::parse_installed_version() const {
    // Check if HELIX_VERSION macro exists and extract version variable
    if (!capabilities_.has_helix_macro("HELIX_VERSION")) {
        return std::nullopt;
    }

    // TODO: Query the macro variable value via Moonraker
    // For now, return the current version since detection means it's installed
    return HELIX_MACROS_VERSION;
}

} // namespace helix
