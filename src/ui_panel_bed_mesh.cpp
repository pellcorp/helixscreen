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

#include "ui_panel_bed_mesh.h"

#include "ui_bed_mesh.h"
#include "ui_nav.h"

#include "app_globals.h"
#include "moonraker_client.h"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstring>
#include <limits>

#include "hv/json.hpp"

using json = nlohmann::json;

// Canvas dimensions (must match ui_bed_mesh widget: 600×400 RGB888)
#define CANVAS_WIDTH 600
#define CANVAS_HEIGHT 400

// Rotation angle ranges
#define ROTATION_X_MIN (-85)
#define ROTATION_X_MAX (-10)
#define ROTATION_X_DEFAULT (-45)
#define ROTATION_Z_MIN 0
#define ROTATION_Z_MAX 360
#define ROTATION_Z_DEFAULT 45

// Static state
static lv_obj_t* canvas = nullptr;
static lv_obj_t* rotation_x_label = nullptr;
static lv_obj_t* rotation_y_label = nullptr;
static lv_obj_t* rotation_x_slider = nullptr;
static lv_obj_t* rotation_z_slider = nullptr;
static lv_obj_t* bed_mesh_panel = nullptr;
static lv_obj_t* parent_obj = nullptr;

// Current rotation angles (for slider state tracking)
static int current_rotation_x = ROTATION_X_DEFAULT;
static int current_rotation_z = ROTATION_Z_DEFAULT;

// Reactive subjects for bed mesh data
static lv_subject_t bed_mesh_available;    // 0 = no mesh, 1 = mesh loaded
static lv_subject_t bed_mesh_profile_name; // String: active profile name
static lv_subject_t bed_mesh_dimensions;   // String: "10x10 points"
static lv_subject_t bed_mesh_z_range;      // String: "Z: 0.05 to 0.35mm"

// String buffers for subjects (LVGL requires persistent buffers)
static char profile_name_buf[64] = "";
static char profile_name_prev_buf[64] = "";
static char dimensions_buf[64] = "No mesh data";
static char dimensions_prev_buf[64] = "";
static char z_range_buf[64] = "";
static char z_range_prev_buf[64] = "";

// Cleanup handler for panel deletion
static void panel_delete_cb(lv_event_t* e) {
    (void)e;

    spdlog::debug("[BedMesh] Panel delete event - cleaning up resources");

    // Widget cleanup (renderer cleanup is handled by widget delete callback)
    canvas = nullptr;
    rotation_x_label = nullptr;
    rotation_y_label = nullptr;
    rotation_x_slider = nullptr;
    rotation_z_slider = nullptr;
    bed_mesh_panel = nullptr;
    parent_obj = nullptr;
}

// Slider event handler: X rotation (tilt)
static void rotation_x_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);

    // Read slider value (0-100)
    int32_t slider_value = lv_slider_get_value(slider);

    // Map to rotation angle range (-85 to -10)
    current_rotation_x = ROTATION_X_MIN + (slider_value * (ROTATION_X_MAX - ROTATION_X_MIN)) / 100;

    // Update label
    if (rotation_x_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Tilt: %d°", current_rotation_x);
        lv_label_set_text(rotation_x_label, buf);
    }

    // Update widget rotation and redraw
    if (canvas) {
        ui_bed_mesh_set_rotation(canvas, current_rotation_x, current_rotation_z);
    }

    spdlog::debug("[BedMesh] X rotation updated: {}°", current_rotation_x);
}

// Slider event handler: Z rotation (spin)
static void rotation_z_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = (lv_obj_t*)lv_event_get_target(e);

    // Read slider value (0-100)
    int32_t slider_value = lv_slider_get_value(slider);

    // Map to rotation angle range (0 to 360)
    current_rotation_z = (slider_value * 360) / 100;

    // Update label
    if (rotation_y_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Spin: %d°", current_rotation_z);
        lv_label_set_text(rotation_y_label, buf);
    }

    // Update widget rotation and redraw
    if (canvas) {
        ui_bed_mesh_set_rotation(canvas, current_rotation_x, current_rotation_z);
    }

    spdlog::debug("[BedMesh] Z rotation updated: {}°", current_rotation_z);
}

// Back button event handler
static void back_button_cb(lv_event_t* e) {
    (void)e;

    // Use navigation history to go back to previous panel
    if (!ui_nav_go_back()) {
        // Fallback: If navigation history is empty, manually hide panel
        if (bed_mesh_panel) {
            lv_obj_add_flag(bed_mesh_panel, LV_OBJ_FLAG_HIDDEN);
        }

        // Show settings panel (typical parent)
        if (parent_obj) {
            lv_obj_t* settings_launcher = lv_obj_find_by_name(parent_obj, "settings_panel");
            if (settings_launcher) {
                lv_obj_clear_flag(settings_launcher, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// Update UI subjects when bed mesh data changes
static void on_bed_mesh_update(const MoonrakerClient::BedMeshProfile& mesh) {
    spdlog::debug("[BedMesh] on_bed_mesh_update called, probed_matrix.size={}",
                  mesh.probed_matrix.size());

    if (mesh.probed_matrix.empty()) {
        lv_subject_set_int(&bed_mesh_available, 0);
        lv_subject_copy_string(&bed_mesh_dimensions, "No mesh data");
        lv_subject_copy_string(&bed_mesh_z_range, "");
        spdlog::warn("[BedMesh] No mesh data available");
        return;
    }

    // Update subjects
    lv_subject_set_int(&bed_mesh_available, 1);

    // Update profile name
    lv_subject_copy_string(&bed_mesh_profile_name, mesh.name.c_str());
    spdlog::debug("[BedMesh] Set profile name: {}", mesh.name);

    // Format and update dimensions
    snprintf(dimensions_buf, sizeof(dimensions_buf), "%dx%d points", mesh.x_count, mesh.y_count);
    lv_subject_copy_string(&bed_mesh_dimensions, dimensions_buf);
    spdlog::debug("[BedMesh] Set dimensions: {}", dimensions_buf);

    // Calculate Z range
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    for (const auto& row : mesh.probed_matrix) {
        for (float z : row) {
            min_z = std::min(min_z, z);
            max_z = std::max(max_z, z);
        }
    }

    // Format and update Z range
    snprintf(z_range_buf, sizeof(z_range_buf), "Z: %.3f to %.3f mm", min_z, max_z);
    lv_subject_copy_string(&bed_mesh_z_range, z_range_buf);
    spdlog::debug("[BedMesh] Set Z range: {}", z_range_buf);

    // Update renderer with new mesh data
    ui_panel_bed_mesh_set_data(mesh.probed_matrix);

    // TEMPORARY: Manually set label text to verify labels are accessible
    lv_obj_t* dim_label = lv_obj_find_by_name(bed_mesh_panel, "mesh_dimensions_label");
    lv_obj_t* range_label = lv_obj_find_by_name(bed_mesh_panel, "mesh_z_range_label");
    if (dim_label) {
        lv_label_set_text(dim_label, dimensions_buf);
        spdlog::debug("[BedMesh] Manually set dimensions label text");
    } else {
        spdlog::warn("[BedMesh] Could not find mesh_dimensions_label");
    }
    if (range_label) {
        lv_label_set_text(range_label, z_range_buf);
        spdlog::debug("[BedMesh] Manually set z_range label text");
    } else {
        spdlog::warn("[BedMesh] Could not find mesh_z_range_label");
    }

    spdlog::info("[BedMesh] Mesh updated: {} ({}x{}, Z: {:.3f} to {:.3f})", mesh.name, mesh.x_count,
                 mesh.y_count, min_z, max_z);
}

void ui_panel_bed_mesh_init_subjects() {
    lv_subject_init_int(&bed_mesh_available, 0);
    lv_subject_init_string(&bed_mesh_profile_name, profile_name_buf, profile_name_prev_buf,
                           sizeof(profile_name_buf), "");
    lv_subject_init_string(&bed_mesh_dimensions, dimensions_buf, dimensions_prev_buf,
                           sizeof(dimensions_buf), "No mesh data");
    lv_subject_init_string(&bed_mesh_z_range, z_range_buf, z_range_prev_buf, sizeof(z_range_buf),
                           "");

    // Register subjects for XML bindings
    lv_xml_register_subject(NULL, "bed_mesh_available", &bed_mesh_available);
    lv_xml_register_subject(NULL, "bed_mesh_profile_name", &bed_mesh_profile_name);
    lv_xml_register_subject(NULL, "bed_mesh_dimensions", &bed_mesh_dimensions);
    lv_xml_register_subject(NULL, "bed_mesh_z_range", &bed_mesh_z_range);

    spdlog::debug("[BedMesh] Subjects initialized and registered");
}

void ui_panel_bed_mesh_setup(lv_obj_t* panel, lv_obj_t* parent_screen) {
    bed_mesh_panel = panel;
    parent_obj = parent_screen;

    spdlog::info("[BedMesh] Setting up event handlers...");

    // Find canvas widget (created by <bed_mesh> XML widget)
    canvas = lv_obj_find_by_name(panel, "bed_mesh_canvas");
    if (!canvas) {
        spdlog::error("[BedMesh] Canvas widget not found in XML");
        return;
    }
    spdlog::debug("[BedMesh] Found canvas widget");

    // Find rotation labels (mesh info labels are now reactively bound)
    rotation_x_label = lv_obj_find_by_name(panel, "rotation_x_label");
    if (!rotation_x_label) {
        spdlog::warn("[BedMesh] X rotation label not found in XML");
    }

    rotation_y_label = lv_obj_find_by_name(panel, "rotation_y_label");
    if (!rotation_y_label) {
        spdlog::warn("[BedMesh] Z rotation label not found in XML");
    }

    // Find rotation sliders
    rotation_x_slider = lv_obj_find_by_name(panel, "rotation_x_slider");
    if (rotation_x_slider) {
        lv_slider_set_range(rotation_x_slider, 0, 100);
        // Map default angle to slider value: (-45 - (-85)) / ((-10) - (-85)) = 40/75 ≈ 53
        int32_t default_x_value =
            ((ROTATION_X_DEFAULT - ROTATION_X_MIN) * 100) / (ROTATION_X_MAX - ROTATION_X_MIN);
        lv_slider_set_value(rotation_x_slider, default_x_value, LV_ANIM_OFF);
        lv_obj_add_event_cb(rotation_x_slider, rotation_x_slider_cb, LV_EVENT_VALUE_CHANGED,
                            nullptr);
        spdlog::debug("[BedMesh] X rotation slider configured (default: {})", default_x_value);
    } else {
        spdlog::warn("[BedMesh] X rotation slider not found in XML");
    }

    rotation_z_slider = lv_obj_find_by_name(panel, "rotation_z_slider");
    if (rotation_z_slider) {
        lv_slider_set_range(rotation_z_slider, 0, 100);
        // Map default angle to slider value: 45 / 360 * 100 ≈ 12.5
        int32_t default_z_value = (ROTATION_Z_DEFAULT * 100) / 360;
        lv_slider_set_value(rotation_z_slider, default_z_value, LV_ANIM_OFF);
        lv_obj_add_event_cb(rotation_z_slider, rotation_z_slider_cb, LV_EVENT_VALUE_CHANGED,
                            nullptr);
        spdlog::debug("[BedMesh] Z rotation slider configured (default: {})", default_z_value);
    } else {
        spdlog::warn("[BedMesh] Z rotation slider not found in XML");
    }

    // Find and setup back button
    lv_obj_t* back_btn = lv_obj_find_by_name(panel, "back_button");
    if (back_btn) {
        lv_obj_add_event_cb(back_btn, back_button_cb, LV_EVENT_CLICKED, nullptr);
        spdlog::debug("[BedMesh] Back button configured");
    } else {
        spdlog::warn("[BedMesh] Back button not found in XML");
    }

    // Canvas buffer and renderer already created by <bed_mesh> widget
    // Widget is initialized with default rotation angles matching our defaults

    // Update rotation labels with initial values
    if (rotation_x_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Tilt: %d°", current_rotation_x);
        lv_label_set_text(rotation_x_label, buf);
    }

    if (rotation_y_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Spin: %d°", current_rotation_z);
        lv_label_set_text(rotation_y_label, buf);
    }

    // Register Moonraker callback for bed mesh updates
    MoonrakerClient* client = get_moonraker_client();
    if (client) {
        client->register_notify_update([client](json notification) {
            // Check if this notification contains bed_mesh updates
            if (notification.contains("params") && notification["params"].is_array() &&
                !notification["params"].empty()) {
                const json& params = notification["params"][0];
                if (params.contains("bed_mesh") && params["bed_mesh"].is_object()) {
                    // Mesh data was updated - refresh UI
                    on_bed_mesh_update(client->get_active_bed_mesh());
                }
            }
        });
        spdlog::debug("[BedMesh] Registered Moonraker callback for mesh updates");
    }

    // Load initial mesh data from MoonrakerClient (mock or real)
    if (client) {
        bool has_mesh = client->has_bed_mesh();
        spdlog::info("[BedMesh] Moonraker client found, has_bed_mesh={}", has_mesh);
        if (has_mesh) {
            const auto& mesh = client->get_active_bed_mesh();
            spdlog::info("[BedMesh] Active mesh: profile='{}', size={}x{}, rows={}", mesh.name,
                         mesh.x_count, mesh.y_count, mesh.probed_matrix.size());
            on_bed_mesh_update(mesh);
        } else {
            spdlog::info("[BedMesh] No mesh data available from Moonraker");
            // Panel will show "No mesh data" via subjects initialized in init_subjects()
        }
    } else {
        spdlog::warn("[BedMesh] Moonraker client is null!");
    }

    // Register cleanup handler
    lv_obj_add_event_cb(panel, panel_delete_cb, LV_EVENT_DELETE, nullptr);

    spdlog::info("[BedMesh] Setup complete!");
}

void ui_panel_bed_mesh_set_data(const std::vector<std::vector<float>>& mesh_data) {
    if (!canvas) {
        spdlog::error("[BedMesh] Cannot set mesh data - canvas not initialized");
        return;
    }

    if (mesh_data.empty() || mesh_data[0].empty()) {
        spdlog::error("[BedMesh] Invalid mesh data - empty rows or columns");
        return;
    }

    int rows = mesh_data.size();
    int cols = mesh_data[0].size();

    // Convert std::vector to C-style array for widget API
    std::vector<const float*> row_pointers(rows);
    for (int i = 0; i < rows; i++) {
        row_pointers[i] = mesh_data[i].data();
    }

    // Set mesh data in widget (automatically triggers redraw)
    if (!ui_bed_mesh_set_data(canvas, row_pointers.data(), rows, cols)) {
        spdlog::error("[BedMesh] Failed to set mesh data in widget");
        return;
    }

    // Update subjects for info labels
    snprintf(dimensions_buf, sizeof(dimensions_buf), "%dx%d points", cols, rows);
    lv_subject_copy_string(&bed_mesh_dimensions, dimensions_buf);

    // Calculate Z range from mesh data
    float min_z = std::numeric_limits<float>::max();
    float max_z = std::numeric_limits<float>::lowest();
    for (const auto& row : mesh_data) {
        for (float val : row) {
            min_z = std::min(min_z, val);
            max_z = std::max(max_z, val);
        }
    }

    snprintf(z_range_buf, sizeof(z_range_buf), "Z: %.3f to %.3f mm", min_z, max_z);
    lv_subject_copy_string(&bed_mesh_z_range, z_range_buf);
}

void ui_panel_bed_mesh_redraw() {
    if (!canvas) {
        spdlog::warn("[BedMesh] Cannot redraw - canvas not initialized");
        return;
    }

    // Trigger redraw via widget API
    ui_bed_mesh_redraw(canvas);
}
