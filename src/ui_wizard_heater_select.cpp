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

#include "ui_wizard_heater_select.h"

#include "ui_wizard.h"
#include "ui_wizard_hardware_selector.h"
#include "ui_wizard_helpers.h"

#include "app_globals.h"
#include "config.h"
#include "lvgl/lvgl.h"
#include "moonraker_client.h"
#include "ui_notification.h"
#include "wizard_config_paths.h"

#include <spdlog/spdlog.h>

#include <cstring>
#include <string>
#include <vector>

// ============================================================================
// Static Data & Subjects
// ============================================================================

// Subject declarations (module scope)
static lv_subject_t bed_heater_selected;
static lv_subject_t hotend_heater_selected;

// Screen instance
static lv_obj_t* heater_select_screen_root = nullptr;

// Dynamic options storage (for event callback mapping)
static std::vector<std::string> bed_heater_items;
static std::vector<std::string> hotend_heater_items;

// ============================================================================
// Subject Initialization
// ============================================================================

void ui_wizard_heater_select_init_subjects() {
    spdlog::debug("[Wizard Heater] Initializing subjects");

    // Initialize subjects with default index 0
    // Actual selection will be restored from config during create() after hardware is discovered
    WizardHelpers::init_int_subject(&bed_heater_selected, 0, "bed_heater_selected");
    WizardHelpers::init_int_subject(&hotend_heater_selected, 0, "hotend_heater_selected");

    spdlog::debug("[Wizard Heater] Subjects initialized");
}

// ============================================================================
// Callback Registration
// ============================================================================

void ui_wizard_heater_select_register_callbacks() {
    // No XML callbacks needed - dropdowns attached programmatically in create()
    spdlog::debug("[Wizard Heater] Callback registration (none needed for hardware selectors)");
}

// ============================================================================
// Screen Creation
// ============================================================================

lv_obj_t* ui_wizard_heater_select_create(lv_obj_t* parent) {
    spdlog::debug("[Wizard Heater] Creating heater select screen");

    // Safety check: cleanup should have been called by wizard navigation
    if (heater_select_screen_root) {
        spdlog::warn(
            "[Wizard Heater] Screen pointer not null - cleanup may not have been called properly");
        heater_select_screen_root = nullptr; // Reset pointer, wizard framework handles deletion
    }

    // Create screen from XML
    heater_select_screen_root = (lv_obj_t*)lv_xml_create(parent, "wizard_heater_select", nullptr);
    if (!heater_select_screen_root) {
        spdlog::error("[Wizard Heater] Failed to create screen from XML");
        ui_notification_error("Wizard Error",
                            "Failed to load heater configuration screen. Please restart the application.");
        return nullptr;
    }

    // Populate bed heater dropdown (discover + filter + populate + restore)
    wizard_populate_hardware_dropdown(
        heater_select_screen_root, "bed_heater_dropdown", &bed_heater_selected, bed_heater_items,
        [](MoonrakerClient* c) -> const auto& { return c->get_heaters(); },
        "bed", // Filter for bed-related heaters
        true,  // Allow "None" option
        WizardConfigPaths::BED_HEATER, [](MoonrakerClient* c) { return c->guess_bed_heater(); },
        "[Wizard Heater]");

    // Attach bed heater dropdown callback programmatically
    lv_obj_t* bed_heater_dropdown =
        lv_obj_find_by_name(heater_select_screen_root, "bed_heater_dropdown");
    if (bed_heater_dropdown) {
        lv_obj_add_event_cb(bed_heater_dropdown, wizard_hardware_dropdown_changed_cb,
                            LV_EVENT_VALUE_CHANGED, &bed_heater_selected);
    }

    // Populate hotend heater dropdown (discover + filter + populate + restore)
    wizard_populate_hardware_dropdown(
        heater_select_screen_root, "hotend_heater_dropdown", &hotend_heater_selected,
        hotend_heater_items, [](MoonrakerClient* c) -> const auto& { return c->get_heaters(); },
        "extruder", // Filter for extruder-related heaters
        true,       // Allow "None" option
        WizardConfigPaths::HOTEND_HEATER,
        [](MoonrakerClient* c) { return c->guess_hotend_heater(); }, "[Wizard Heater]");

    // Attach hotend heater dropdown callback programmatically
    lv_obj_t* hotend_heater_dropdown =
        lv_obj_find_by_name(heater_select_screen_root, "hotend_heater_dropdown");
    if (hotend_heater_dropdown) {
        lv_obj_add_event_cb(hotend_heater_dropdown, wizard_hardware_dropdown_changed_cb,
                            LV_EVENT_VALUE_CHANGED, &hotend_heater_selected);
    }

    spdlog::debug("[Wizard Heater] Screen created successfully");
    return heater_select_screen_root;
}

// ============================================================================
// Cleanup
// ============================================================================

void ui_wizard_heater_select_cleanup() {
    spdlog::debug("[Wizard Heater] Cleaning up resources");

    Config* config = Config::get_instance();
    if (!config) {
        spdlog::error("[Wizard Heater] Config instance not available!");
        return;
    }

    // Save bed heater selection
    // Store the heater name to BOTH heater and sensor paths (Klipper heaters provide temp readings)
    WizardHelpers::save_dropdown_selection(&bed_heater_selected, bed_heater_items,
                                           WizardConfigPaths::BED_HEATER, "[Wizard Heater]");

    // Get the selected bed heater name and also save it as the sensor
    int32_t bed_idx = lv_subject_get_int(&bed_heater_selected);
    if (bed_idx >= 0 && bed_idx < static_cast<int32_t>(bed_heater_items.size())) {
        const std::string& bed_heater_name = bed_heater_items[bed_idx];
        config->set<std::string>(WizardConfigPaths::BED_SENSOR, bed_heater_name);
        spdlog::debug("[Wizard Heater] Bed sensor set to: {}", bed_heater_name);
    }

    // Save hotend heater selection
    // Store the heater name to BOTH heater and sensor paths
    WizardHelpers::save_dropdown_selection(&hotend_heater_selected, hotend_heater_items,
                                           WizardConfigPaths::HOTEND_HEATER, "[Wizard Heater]");

    // Get the selected hotend heater name and also save it as the sensor
    int32_t hotend_idx = lv_subject_get_int(&hotend_heater_selected);
    if (hotend_idx >= 0 && hotend_idx < static_cast<int32_t>(hotend_heater_items.size())) {
        const std::string& hotend_heater_name = hotend_heater_items[hotend_idx];
        config->set<std::string>(WizardConfigPaths::HOTEND_SENSOR, hotend_heater_name);
        spdlog::debug("[Wizard Heater] Hotend sensor set to: {}", hotend_heater_name);
    }

    // Persist to disk
    if (!config->save()) {
        spdlog::error("[Wizard Heater] Failed to save heater configuration to disk!");
    }

    // Reset UI references
    // Note: Do NOT call lv_obj_del() here - the wizard framework handles
    // object deletion when clearing wizard_content container
    heater_select_screen_root = nullptr;

    spdlog::debug("[Wizard Heater] Cleanup complete");
}

// ============================================================================
// Validation
// ============================================================================

bool ui_wizard_heater_select_is_validated() {
    // Always return true for baseline implementation
    return true;
}
