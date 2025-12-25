// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 HelixScreen Contributors
#pragma once

/**
 * @file ui_temperature_utils.h
 * @brief Shared temperature validation and safety utilities
 *
 * This module provides centralized temperature validation, clamping, and
 * safety checking logic used across multiple temperature-related panels
 * (controls/temp, filament, extrusion).
 */

namespace helix {
namespace ui {
namespace temperature {

// ============================================================================
// Unit Conversion Functions
// ============================================================================

/**
 * @brief Converts centidegrees to degrees (integer)
 *
 * PrinterState stores temperatures as centidegrees (×10) for 0.1°C resolution.
 * Use this function for integer display (e.g., "210°C").
 *
 * @param centi Temperature in centidegrees (e.g., 2100 for 210°C)
 * @return Temperature in degrees (e.g., 210)
 */
inline int centi_to_degrees(int centi) {
    return centi / 10;
}

/**
 * @brief Converts centidegrees to degrees (float for precision display)
 *
 * Use this function when 0.1°C precision is needed (e.g., graph data points).
 *
 * @param centi Temperature in centidegrees (e.g., 2105 for 210.5°C)
 * @return Temperature in degrees (e.g., 210.5f)
 */
inline float centi_to_degrees_f(int centi) {
    return static_cast<float>(centi) / 10.0f;
}

/**
 * @brief Converts degrees to centidegrees
 *
 * Use when setting temperatures from user input (e.g., keyboard entry).
 *
 * @param degrees Temperature in degrees (e.g., 210)
 * @return Temperature in centidegrees (e.g., 2100)
 */
inline int degrees_to_centi(int degrees) {
    return degrees * 10;
}

// ============================================================================
// Validation Functions
// ============================================================================

/**
 * @brief Validates and clamps a temperature value to safe limits
 *
 * If the temperature is outside the valid range, it will be clamped to
 * the nearest valid value and a warning will be logged.
 *
 * @param temp Temperature value to validate (modified in-place if clamped)
 * @param min_temp Minimum valid temperature
 * @param max_temp Maximum valid temperature
 * @param context Logging context (e.g., "Temp", "Filament", "Extrusion")
 * @param temp_type Temperature type for logging (e.g., "current", "target")
 * @return true if temperature was valid, false if it was clamped
 */
bool validate_and_clamp(int& temp, int min_temp, int max_temp, const char* context,
                        const char* temp_type);

/**
 * @brief Validates and clamps a temperature pair (current + target)
 *
 * Convenience function that validates both current and target temperatures.
 *
 * @param current Current temperature (modified in-place if clamped)
 * @param target Target temperature (modified in-place if clamped)
 * @param min_temp Minimum valid temperature
 * @param max_temp Maximum valid temperature
 * @param context Logging context (e.g., "Temp", "Filament")
 * @return true if both temperatures were valid, false if either was clamped
 */
bool validate_and_clamp_pair(int& current, int& target, int min_temp, int max_temp,
                             const char* context);

/**
 * @brief Checks if the current temperature is safe for extrusion
 *
 * Extrusion operations require the nozzle to be at or above a minimum
 * temperature (typically 170°C) to avoid damaging the extruder.
 *
 * @param current_temp Current nozzle temperature
 * @param min_extrusion_temp Minimum safe extrusion temperature
 * @return true if safe to extrude, false otherwise
 */
bool is_extrusion_safe(int current_temp, int min_extrusion_temp);

/**
 * @brief Gets a human-readable safety status message
 *
 * @param current_temp Current nozzle temperature
 * @param min_extrusion_temp Minimum safe extrusion temperature
 * @return Status message (e.g., "Ready" or "Heating (45°C below minimum)")
 */
const char* get_extrusion_safety_status(int current_temp, int min_extrusion_temp);

} // namespace temperature
} // namespace ui
} // namespace helix
