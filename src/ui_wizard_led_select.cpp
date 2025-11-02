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

#include "ui_wizard_led_select.h"
#include "ui_wizard.h"
#include "app_globals.h"
#include "config.h"
#include "lvgl/lvgl.h"
#include <spdlog/spdlog.h>
#include <string>
#include <cstring>

// ============================================================================
// Static Data & Subjects
// ============================================================================

// Subject declarations (module scope)
static lv_subject_t led_strip_selected;

// Screen instance
static lv_obj_t* led_select_screen_root = nullptr;

// Placeholder options for dropdown
static const char* led_strip_options = "neopixel my_neopixel\ndotstar my_dotstar\nNone";

// ============================================================================
// Forward Declarations
// ============================================================================

static void on_led_strip_changed(lv_event_t* e);

// ============================================================================
// Subject Initialization
// ============================================================================

void ui_wizard_led_select_init_subjects() {
    spdlog::debug("[Wizard LED] Initializing subjects");

    // Load existing values from config if available
    Config* config = Config::get_instance();

    // Initialize LED strip selection (default to None)
    int32_t led_index = 2;  // Default to "None"
    if (config) {
        std::string led = config->get<std::string>("/printer/led_strip", "None");
        if (led == "neopixel my_neopixel") {
            led_index = 0;
        } else if (led == "dotstar my_dotstar") {
            led_index = 1;
        }
    }
    lv_subject_init_int(&led_strip_selected, led_index);
    lv_xml_register_subject(nullptr, "led_strip_selected", &led_strip_selected);

    // Always enable Next button for baseline implementation
    ui_wizard_set_button_enabled(true, true);

    spdlog::info("[Wizard LED] Subjects initialized - LED strip: {}", led_index);
}

// ============================================================================
// Event Callbacks
// ============================================================================

static void on_led_strip_changed(lv_event_t* e) {
    lv_obj_t* dropdown = (lv_obj_t*)lv_event_get_target(e);
    uint16_t selected_index = lv_dropdown_get_selected(dropdown);

    spdlog::debug("[Wizard LED] LED strip selection changed to index: {}", selected_index);

    // Update subject
    lv_subject_set_int(&led_strip_selected, selected_index);

    // Save to config
    Config* config = Config::get_instance();
    if (config) {
        const char* options[] = {"neopixel my_neopixel", "dotstar my_dotstar", "None"};
        if (selected_index < sizeof(options)/sizeof(options[0])) {
            config->set("/printer/led_strip", std::string(options[selected_index]));
            spdlog::debug("[Wizard LED] Saved LED strip: {}", options[selected_index]);
        }
    }
}

// ============================================================================
// Callback Registration
// ============================================================================

void ui_wizard_led_select_register_callbacks() {
    spdlog::debug("[Wizard LED] Registering callbacks");

    lv_xml_register_event_cb(nullptr, "on_led_strip_changed", on_led_strip_changed);
}

// ============================================================================
// Screen Creation
// ============================================================================

lv_obj_t* ui_wizard_led_select_create(lv_obj_t* parent) {
    spdlog::info("[Wizard LED] Creating LED select screen");

    if (led_select_screen_root) {
        spdlog::warn("[Wizard LED] Screen already exists, destroying old instance");
        lv_obj_del(led_select_screen_root);
        led_select_screen_root = nullptr;
    }

    // Create screen from XML
    led_select_screen_root = (lv_obj_t*)lv_xml_create(parent, "wizard_led_select", nullptr);
    if (!led_select_screen_root) {
        spdlog::error("[Wizard LED] Failed to create screen from XML");
        return nullptr;
    }

    // Find and configure LED strip dropdown
    lv_obj_t* led_dropdown = lv_obj_find_by_name(led_select_screen_root, "led_strip_dropdown");
    if (led_dropdown) {
        lv_dropdown_set_options(led_dropdown, led_strip_options);
        int index = lv_subject_get_int(&led_strip_selected);
        lv_dropdown_set_selected(led_dropdown, index);
        spdlog::debug("[Wizard LED] Configured LED dropdown with {} options, selected: {}",
                     3, index);
    }

    spdlog::info("[Wizard LED] Screen created successfully");
    return led_select_screen_root;
}

// ============================================================================
// Cleanup
// ============================================================================

void ui_wizard_led_select_cleanup() {
    spdlog::debug("[Wizard LED] Cleaning up resources");

    if (led_select_screen_root) {
        lv_obj_del(led_select_screen_root);
        led_select_screen_root = nullptr;
    }
}

// ============================================================================
// Validation
// ============================================================================

bool ui_wizard_led_select_is_validated() {
    // Always return true for baseline implementation
    return true;
}