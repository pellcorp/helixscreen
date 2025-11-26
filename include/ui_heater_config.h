// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 HelixScreen Contributors
/**
 * @file ui_heater_config.h
 * @brief Shared heater configuration structure and utilities
 *
 * This module provides a unified configuration structure for heaters (nozzle, bed)
 * and helper functions to eliminate duplicate setup code across temperature panels.
 */

#ifndef UI_HEATER_CONFIG_H
#define UI_HEATER_CONFIG_H

#include "lvgl/lvgl.h"

/**
 * @brief Heater type enumeration
 */
typedef enum {
    HEATER_NOZZLE, ///< Hotend/nozzle heater
    HEATER_BED     ///< Heated bed
} heater_type_t;

/**
 * @brief Heater configuration structure
 *
 * This structure encapsulates all configuration needed for a heater panel,
 * including display colors, temperature ranges, presets, and keypad ranges.
 */
typedef struct {
    heater_type_t type;      ///< Heater type (nozzle or bed)
    const char* name;        ///< Short name (e.g., "nozzle", "bed")
    const char* title;       ///< Display title (e.g., "Nozzle Temperature")
    lv_color_t color;        ///< Theme color for this heater
    float temp_range_max;    ///< Maximum temperature for graph Y-axis
    int y_axis_increment;    ///< Y-axis label increment (e.g., 50°C, 100°C)

    struct {
        int off;  ///< "Off" preset (0°C)
        int pla;  ///< PLA preset
        int petg; ///< PETG preset
        int abs;  ///< ABS preset
    } presets;

    struct {
        float min; ///< Minimum keypad input value
        float max; ///< Maximum keypad input value
    } keypad_range;
} heater_config_t;

#endif // UI_HEATER_CONFIG_H
