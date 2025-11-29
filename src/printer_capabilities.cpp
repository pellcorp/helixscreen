// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "printer_capabilities.h"

#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <sstream>

// ============================================================================
// Parsing
// ============================================================================

void PrinterCapabilities::parse_objects(const json& objects) {
    clear();

    for (const auto& obj : objects) {
        std::string name = obj.template get<std::string>();
        std::string upper_name = to_upper(name);

        // Hardware detection
        if (name == "quad_gantry_level") {
            has_qgl_ = true;
            spdlog::debug("[PrinterCapabilities] Detected quad_gantry_level");
        } else if (name == "z_tilt") {
            has_z_tilt_ = true;
            spdlog::debug("[PrinterCapabilities] Detected z_tilt");
        } else if (name == "bed_mesh") {
            has_bed_mesh_ = true;
            spdlog::debug("[PrinterCapabilities] Detected bed_mesh");
        }
        // Chamber heater detection (heater_generic with "chamber" in name)
        else if (name.rfind("heater_generic ", 0) == 0) {
            std::string heater_name = name.substr(15);  // Remove "heater_generic " prefix
            if (to_upper(heater_name).find("CHAMBER") != std::string::npos) {
                has_chamber_heater_ = true;
                spdlog::debug("[PrinterCapabilities] Detected chamber heater: {}", name);
            }
        }
        // Chamber sensor detection
        else if (name.rfind("temperature_sensor ", 0) == 0) {
            std::string sensor_name = name.substr(19);  // Remove "temperature_sensor " prefix
            if (to_upper(sensor_name).find("CHAMBER") != std::string::npos) {
                has_chamber_sensor_ = true;
                spdlog::debug("[PrinterCapabilities] Detected chamber sensor: {}", name);
            }
        }
        // Macro detection
        else if (name.rfind("gcode_macro ", 0) == 0) {
            std::string macro_name = name.substr(12);  // Remove "gcode_macro " prefix
            std::string upper_macro = to_upper(macro_name);

            macros_.insert(upper_macro);

            // Check for HelixScreen helper macros
            if (upper_macro.rfind("HELIX_", 0) == 0) {
                helix_macros_.insert(upper_macro);
                spdlog::debug("[PrinterCapabilities] Detected HelixScreen macro: {}", macro_name);
            }

            // Check for common macro patterns and cache them
            if (nozzle_clean_macro_.empty()) {
                static const std::vector<std::string> nozzle_patterns = {
                    "CLEAN_NOZZLE", "NOZZLE_WIPE", "WIPE_NOZZLE", "PURGE_NOZZLE", "NOZZLE_CLEAN"};
                if (matches_any(upper_macro, nozzle_patterns)) {
                    nozzle_clean_macro_ = macro_name;
                    spdlog::debug("[PrinterCapabilities] Detected nozzle clean macro: {}",
                                  macro_name);
                }
            }

            if (purge_line_macro_.empty()) {
                static const std::vector<std::string> purge_patterns = {
                    "PURGE_LINE", "PRIME_LINE", "INTRO_LINE", "LINE_PURGE"};
                if (matches_any(upper_macro, purge_patterns)) {
                    purge_line_macro_ = macro_name;
                    spdlog::debug("[PrinterCapabilities] Detected purge line macro: {}",
                                  macro_name);
                }
            }

            if (heat_soak_macro_.empty()) {
                static const std::vector<std::string> soak_patterns = {"HEAT_SOAK", "CHAMBER_SOAK",
                                                                        "SOAK", "BED_SOAK"};
                if (matches_any(upper_macro, soak_patterns)) {
                    heat_soak_macro_ = macro_name;
                    spdlog::debug("[PrinterCapabilities] Detected heat soak macro: {}", macro_name);
                }
            }
        }
    }

    spdlog::info("[PrinterCapabilities] {}", summary());
}

void PrinterCapabilities::clear() {
    has_qgl_ = false;
    has_z_tilt_ = false;
    has_bed_mesh_ = false;
    has_chamber_heater_ = false;
    has_chamber_sensor_ = false;
    macros_.clear();
    helix_macros_.clear();
    nozzle_clean_macro_.clear();
    purge_line_macro_.clear();
    heat_soak_macro_.clear();
}

// ============================================================================
// Macro Queries
// ============================================================================

bool PrinterCapabilities::has_macro(const std::string& macro_name) const {
    return macros_.count(to_upper(macro_name)) > 0;
}

bool PrinterCapabilities::has_helix_macro(const std::string& macro_name) const {
    return helix_macros_.count(to_upper(macro_name)) > 0;
}

bool PrinterCapabilities::has_nozzle_clean_macro() const {
    return !nozzle_clean_macro_.empty();
}

bool PrinterCapabilities::has_purge_line_macro() const {
    return !purge_line_macro_.empty();
}

bool PrinterCapabilities::has_heat_soak_macro() const {
    return !heat_soak_macro_.empty();
}

std::string PrinterCapabilities::get_nozzle_clean_macro() const {
    return nozzle_clean_macro_;
}

std::string PrinterCapabilities::get_purge_line_macro() const {
    return purge_line_macro_;
}

std::string PrinterCapabilities::get_heat_soak_macro() const {
    return heat_soak_macro_;
}

// ============================================================================
// Summary
// ============================================================================

std::string PrinterCapabilities::summary() const {
    std::ostringstream ss;
    ss << "Capabilities: ";

    std::vector<std::string> caps;

    if (has_qgl_)
        caps.push_back("QGL");
    if (has_z_tilt_)
        caps.push_back("Z-tilt");
    if (has_bed_mesh_)
        caps.push_back("bed_mesh");
    if (has_chamber_heater_)
        caps.push_back("chamber_heater");
    if (has_chamber_sensor_)
        caps.push_back("chamber_sensor");

    if (caps.empty()) {
        ss << "none";
    } else {
        for (size_t i = 0; i < caps.size(); ++i) {
            if (i > 0)
                ss << ", ";
            ss << caps[i];
        }
    }

    ss << " | " << macros_.size() << " macros";
    if (!helix_macros_.empty()) {
        ss << " (" << helix_macros_.size() << " HELIX_*)";
    }

    return ss.str();
}

// ============================================================================
// Helpers
// ============================================================================

std::string PrinterCapabilities::to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    return result;
}

bool PrinterCapabilities::matches_any(const std::string& name,
                                       const std::vector<std::string>& patterns) {
    for (const auto& pattern : patterns) {
        if (name == pattern) {
            return true;
        }
    }
    return false;
}
