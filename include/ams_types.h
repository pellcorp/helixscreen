// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file ams_types.h
 * @brief Data structures for multi-filament/AMS system support
 *
 * Supports both Happy Hare (MMU) and AFC-Klipper-Add-On systems.
 * These structures are platform-agnostic - backends translate from
 * their specific APIs to these common types.
 *
 * @note Thread Safety: These structures are NOT thread-safe. The AmsState
 * class provides thread-safe access through LVGL subjects. Direct mutation
 * of these structures should only occur in the backend layer.
 */

/// Default color for gates without filament info (medium gray)
constexpr uint32_t AMS_DEFAULT_GATE_COLOR = 0x808080;

/**
 * @brief Type of AMS system detected
 */
enum class AmsType {
    NONE = 0,       ///< No AMS detected
    HAPPY_HARE = 1, ///< Happy Hare MMU (mmu object in Moonraker)
    AFC = 2         ///< AFC-Klipper-Add-On (afc object, lane_data database)
};

/**
 * @brief Get string name for AMS type
 * @param type The AMS type enum value
 * @return Human-readable string for the type
 */
inline const char* ams_type_to_string(AmsType type) {
    switch (type) {
    case AmsType::HAPPY_HARE:
        return "Happy Hare";
    case AmsType::AFC:
        return "AFC";
    default:
        return "None";
    }
}

/**
 * @brief Parse AMS type from string (for Moonraker responses)
 * @param str String to parse (case-insensitive)
 * @return Matching AmsType or NONE if not recognized
 */
inline AmsType ams_type_from_string(std::string_view str) {
    // Simple comparison - backends will use their own detection
    if (str == "mmu" || str == "happy_hare" || str == "Happy Hare") {
        return AmsType::HAPPY_HARE;
    }
    if (str == "afc" || str == "AFC") {
        return AmsType::AFC;
    }
    return AmsType::NONE;
}

/**
 * @brief Gate/Lane status
 *
 * Our internal status representation. Use conversion functions to
 * translate from Happy Hare's gate_status values (-1, 0, 1, 2).
 */
enum class GateStatus {
    UNKNOWN = 0,     ///< Status not known
    EMPTY = 1,       ///< No filament in gate
    AVAILABLE = 2,   ///< Filament available, not loaded
    LOADED = 3,      ///< Filament loaded to extruder
    FROM_BUFFER = 4, ///< Filament available from buffer
    BLOCKED = 5      ///< Gate blocked/jammed
};

/**
 * @brief Get string name for gate status
 * @param status The gate status enum value
 * @return Human-readable string for the status
 */
inline const char* gate_status_to_string(GateStatus status) {
    switch (status) {
    case GateStatus::EMPTY:
        return "Empty";
    case GateStatus::AVAILABLE:
        return "Available";
    case GateStatus::LOADED:
        return "Loaded";
    case GateStatus::FROM_BUFFER:
        return "From Buffer";
    case GateStatus::BLOCKED:
        return "Blocked";
    default:
        return "Unknown";
    }
}

/**
 * @brief Convert Happy Hare gate_status integer to GateStatus enum
 *
 * Happy Hare uses: -1 = unknown, 0 = empty, 1 = available, 2 = from buffer
 * The "loaded" state is determined by comparing with current_gate, not from
 * gate_status directly.
 *
 * @param hh_status Happy Hare gate_status value (-1, 0, 1, or 2)
 * @return Corresponding GateStatus enum value
 */
inline GateStatus gate_status_from_happy_hare(int hh_status) {
    switch (hh_status) {
    case -1:
        return GateStatus::UNKNOWN;
    case 0:
        return GateStatus::EMPTY;
    case 1:
        return GateStatus::AVAILABLE;
    case 2:
        return GateStatus::FROM_BUFFER;
    default:
        return GateStatus::UNKNOWN;
    }
}

/**
 * @brief Convert GateStatus enum to Happy Hare gate_status integer
 * @param status Our GateStatus enum value
 * @return Happy Hare gate_status value (-1, 0, 1, or 2)
 */
inline int gate_status_to_happy_hare(GateStatus status) {
    switch (status) {
    case GateStatus::UNKNOWN:
        return -1;
    case GateStatus::EMPTY:
        return 0;
    case GateStatus::AVAILABLE:
        return 1;
    case GateStatus::FROM_BUFFER:
        return 2;
    // LOADED and BLOCKED don't have direct HH equivalents
    case GateStatus::LOADED:
        return 1; // Treat as available
    case GateStatus::BLOCKED:
        return -1; // Treat as unknown
    default:
        return -1;
    }
}

/**
 * @brief Current AMS action/operation
 *
 * Maps to Happy Hare's action strings:
 * "Idle", "Loading", "Unloading", "Forming Tip", "Heating", etc.
 */
enum class AmsAction {
    IDLE = 0,        ///< No operation in progress
    LOADING = 1,     ///< Loading filament to extruder
    UNLOADING = 2,   ///< Unloading filament from extruder
    SELECTING = 3,   ///< Selecting tool/gate
    HOMING = 4,      ///< Homing selector
    FORMING_TIP = 5, ///< Forming filament tip for retraction
    HEATING = 6,     ///< Heating for operation
    CHECKING = 7,    ///< Checking gates
    PAUSED = 8,      ///< Operation paused (requires attention)
    ERROR = 9        ///< Error state
};

/**
 * @brief Get string name for AMS action
 * @param action The action enum value
 * @return Human-readable string for the action
 */
inline const char* ams_action_to_string(AmsAction action) {
    switch (action) {
    case AmsAction::IDLE:
        return "Idle";
    case AmsAction::LOADING:
        return "Loading";
    case AmsAction::UNLOADING:
        return "Unloading";
    case AmsAction::SELECTING:
        return "Selecting";
    case AmsAction::HOMING:
        return "Homing";
    case AmsAction::FORMING_TIP:
        return "Forming Tip";
    case AmsAction::HEATING:
        return "Heating";
    case AmsAction::CHECKING:
        return "Checking";
    case AmsAction::PAUSED:
        return "Paused";
    case AmsAction::ERROR:
        return "Error";
    default:
        return "Unknown";
    }
}

/**
 * @brief Parse AMS action from Happy Hare action string
 * @param action_str Action string from printer.mmu.action
 * @return Corresponding AmsAction enum value
 */
inline AmsAction ams_action_from_string(std::string_view action_str) {
    if (action_str == "Idle")
        return AmsAction::IDLE;
    if (action_str == "Loading")
        return AmsAction::LOADING;
    if (action_str == "Unloading")
        return AmsAction::UNLOADING;
    if (action_str == "Selecting")
        return AmsAction::SELECTING;
    if (action_str == "Homing")
        return AmsAction::HOMING;
    if (action_str == "Forming Tip")
        return AmsAction::FORMING_TIP;
    if (action_str == "Heating")
        return AmsAction::HEATING;
    if (action_str == "Checking")
        return AmsAction::CHECKING;
    // Happy Hare uses "Paused" for attention-required states
    if (action_str.find("Pause") != std::string_view::npos)
        return AmsAction::PAUSED;
    if (action_str.find("Error") != std::string_view::npos)
        return AmsAction::ERROR;
    return AmsAction::IDLE;
}

/**
 * @brief Information about a single gate/lane
 *
 * This represents one filament slot in an AMS unit.
 * Happy Hare calls these "gates", AFC calls them "lanes".
 */
struct GateInfo {
    int gate_index = -1;   ///< Gate/lane number (0-based within unit)
    int global_index = -1; ///< Global index across all units
    GateStatus status = GateStatus::UNKNOWN;

    // Filament information
    std::string color_name;                      ///< Named color (e.g., "Red", "Blue")
    uint32_t color_rgb = AMS_DEFAULT_GATE_COLOR; ///< RGB color for UI (0xRRGGBB)
    std::string material;                        ///< Material type (e.g., "PLA", "PETG", "ABS")
    std::string brand;                           ///< Brand name (e.g., "Polymaker", "eSUN")

    // Temperature recommendations (from Spoolman or manual entry)
    int nozzle_temp_min = 0; ///< Minimum nozzle temp (°C)
    int nozzle_temp_max = 0; ///< Maximum nozzle temp (°C)
    int bed_temp = 0;        ///< Recommended bed temp (°C)

    // Tool mapping
    int mapped_tool = -1; ///< Which tool this gate maps to (-1=none)

    // Spoolman integration
    int spoolman_id = 0;           ///< Spoolman spool ID (0=not tracked)
    std::string spool_name;        ///< Spool name from Spoolman
    float remaining_weight_g = -1; ///< Remaining filament weight in grams (-1=unknown)
    float total_weight_g = -1;     ///< Total spool weight in grams (-1=unknown)

    // Endless spool support (Happy Hare)
    int endless_spool_group = -1; ///< Endless spool group (-1=not grouped)

    /**
     * @brief Get remaining percentage
     * @return 0-100 or -1 if unknown
     */
    [[nodiscard]] float get_remaining_percent() const {
        if (remaining_weight_g < 0 || total_weight_g <= 0)
            return -1;
        return (remaining_weight_g / total_weight_g) * 100.0f;
    }

    /**
     * @brief Check if this gate has filament data configured
     * @return true if material or custom color is set
     */
    [[nodiscard]] bool has_filament_info() const {
        return !material.empty() || color_rgb != AMS_DEFAULT_GATE_COLOR;
    }
};

/**
 * @brief Information about an AMS unit
 *
 * Supports multi-unit configurations (e.g., 2x Box Turtles = 16 slots).
 * Most setups have a single unit with 4-8 gates.
 */
struct AmsUnit {
    int unit_index = 0;              ///< Unit number (0-based)
    std::string name;                ///< Unit name/identifier (e.g., "MMU", "Box Turtle 1")
    int gate_count = 0;              ///< Number of gates on this unit
    int first_gate_global_index = 0; ///< Global index of first gate

    std::vector<GateInfo> gates; ///< Gate information

    // Unit-level status
    bool connected = false;       ///< Unit communication status
    std::string firmware_version; ///< Firmware version if available

    // Sensors (Happy Hare)
    bool has_encoder = false;         ///< Has filament encoder
    bool has_toolhead_sensor = false; ///< Has toolhead filament sensor
    bool has_gate_sensors = false;    ///< Has per-gate sensors

    /**
     * @brief Get gate by local index (within this unit)
     * @param local_index Index within this unit (0 to gate_count-1)
     * @return Pointer to gate info or nullptr if out of range
     */
    [[nodiscard]] const GateInfo* get_gate(int local_index) const {
        if (local_index < 0 || local_index >= static_cast<int>(gates.size())) {
            return nullptr;
        }
        return &gates[local_index];
    }

    /**
     * @brief Get mutable gate by local index (within this unit)
     * @param local_index Index within this unit (0 to gate_count-1)
     * @return Pointer to gate info or nullptr if out of range
     */
    [[nodiscard]] GateInfo* get_gate(int local_index) {
        if (local_index < 0 || local_index >= static_cast<int>(gates.size())) {
            return nullptr;
        }
        return &gates[local_index];
    }
};

/**
 * @brief Complete AMS system state
 *
 * This is the top-level structure containing all AMS information.
 */
struct AmsSystemInfo {
    AmsType type = AmsType::NONE;
    std::string type_name; ///< "Happy Hare", "AFC", etc.
    std::string version;   ///< System version string

    // Current state
    int current_tool = -1;              ///< Active tool (-1=none, -2=bypass for HH)
    int current_gate = -1;              ///< Active gate (-1=none, -2=bypass for HH)
    bool filament_loaded = false;       ///< Filament at extruder
    AmsAction action = AmsAction::IDLE; ///< Current operation
    std::string operation_detail;       ///< Detailed operation string

    // Units
    std::vector<AmsUnit> units; ///< All AMS units
    int total_gates = 0;        ///< Sum of all gates across units

    // Capability flags
    bool supports_endless_spool = false;
    bool supports_spoolman = false;
    bool supports_tool_mapping = false;
    bool supports_bypass = false; ///< Has bypass selector position

    // Tool-to-gate mapping (Happy Hare)
    std::vector<int> tool_to_gate_map; ///< tool_to_gate_map[tool] = gate

    /**
     * @brief Get gate by global index (across all units)
     * @param global_index Global gate index (0 to total_gates-1)
     * @return Pointer to gate info or nullptr if out of range
     */
    [[nodiscard]] const GateInfo* get_gate_global(int global_index) const {
        for (const auto& unit : units) {
            if (global_index >= unit.first_gate_global_index &&
                global_index < unit.first_gate_global_index + unit.gate_count) {
                int local_idx = global_index - unit.first_gate_global_index;
                return unit.get_gate(local_idx);
            }
        }
        return nullptr;
    }

    /**
     * @brief Get mutable gate by global index (across all units)
     * @param global_index Global gate index (0 to total_gates-1)
     * @return Pointer to gate info or nullptr if out of range
     */
    [[nodiscard]] GateInfo* get_gate_global(int global_index) {
        for (auto& unit : units) {
            if (global_index >= unit.first_gate_global_index &&
                global_index < unit.first_gate_global_index + unit.gate_count) {
                int local_idx = global_index - unit.first_gate_global_index;
                return unit.get_gate(local_idx);
            }
        }
        return nullptr;
    }

    /**
     * @brief Get the currently active gate info
     * @return Pointer to active gate or nullptr if none selected
     */
    [[nodiscard]] const GateInfo* get_active_gate() const {
        if (current_gate < 0)
            return nullptr;
        return get_gate_global(current_gate);
    }

    /**
     * @brief Check if system is available and connected
     * @return true if AMS type is detected and has at least one unit
     */
    [[nodiscard]] bool is_available() const {
        return type != AmsType::NONE && !units.empty();
    }

    /**
     * @brief Check if an operation is in progress
     * @return true if actively loading, unloading, etc.
     */
    [[nodiscard]] bool is_busy() const {
        return action != AmsAction::IDLE && action != AmsAction::ERROR;
    }
};

/**
 * @brief Filament requirement from G-code analysis
 *
 * Used for print preview to show which colors are needed.
 */
struct FilamentRequirement {
    int tool_index = -1;                         ///< Tool number from G-code (T0, T1, etc.)
    uint32_t color_rgb = AMS_DEFAULT_GATE_COLOR; ///< Color hint from slicer
    std::string material;                        ///< Material hint from slicer (if available)
    int mapped_gate = -1;                        ///< Which gate is mapped to this tool

    /**
     * @brief Check if this requirement is satisfied by a gate
     * @return true if a gate is mapped to this tool
     */
    [[nodiscard]] bool is_satisfied() const {
        return mapped_gate >= 0;
    }
};

/**
 * @brief Print color requirements summary
 */
struct PrintColorInfo {
    std::vector<FilamentRequirement> requirements;
    int initial_tool = 0;       ///< First tool used in print
    bool all_satisfied = false; ///< All requirements have mapped gates
};
