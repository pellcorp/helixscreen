// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_current_tool.cpp
 * @brief C++ backing for the ams_current_tool XML component
 *
 * Handles:
 * - Color swatch updates (observing ams_current_color subject)
 * - Click handler (opens AMS panel)
 * - Cleanup on widget deletion
 */

#include "ui_nav_manager.h"
#include "ui_observer_guard.h"
#include "ui_panel_ams.h"

#include "ams_state.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/xml/lv_xml.h"

#include <spdlog/spdlog.h>

#include <memory>
#include <unordered_map>

namespace {

// Per-widget data for ams_current_tool instances
struct AmsCurrentToolData {
    lv_obj_t* color_swatch = nullptr;
    ObserverGuard color_observer;
};

// Registry for cleanup
static std::unordered_map<lv_obj_t*, std::unique_ptr<AmsCurrentToolData>> s_registry;

// Observer callback when color subject changes
static void on_color_changed(lv_observer_t* observer, lv_subject_t* subject) {
    lv_obj_t* widget = static_cast<lv_obj_t*>(lv_observer_get_user_data(observer));
    if (!widget)
        return;

    auto it = s_registry.find(widget);
    if (it == s_registry.end() || !it->second->color_swatch)
        return;

    int color_int = lv_subject_get_int(subject);
    lv_color_t color = lv_color_hex(static_cast<uint32_t>(color_int));
    lv_obj_set_style_bg_color(it->second->color_swatch, color, 0);

    spdlog::trace("[AmsCurrentTool] Color updated to 0x{:06X}", color_int);
}

// Cleanup callback when widget is deleted
static void on_delete(lv_event_t* e) {
    lv_obj_t* widget = lv_event_get_target_obj(e);
    auto it = s_registry.find(widget);
    if (it != s_registry.end()) {
        // Release observer before deleting data
        it->second->color_observer.release();
        s_registry.erase(it);
        spdlog::debug("[AmsCurrentTool] Widget cleaned up");
    }
}

// Click callback - opens AMS panel
static void on_clicked(lv_event_t* e) {
    (void)e;
    spdlog::debug("[AmsCurrentTool] Clicked - opening AMS panel");

    auto& ams_panel = get_global_ams_panel();
    if (!ams_panel.are_subjects_initialized()) {
        ams_panel.init_subjects();
    }
    lv_obj_t* panel_obj = ams_panel.get_panel();
    if (panel_obj) {
        ui_nav_push_overlay(panel_obj);
    }
}

// Post-create hook called after XML creates the widget
static void on_widget_created(lv_obj_t* widget) {
    if (!widget)
        return;

    // Create per-widget data
    auto data = std::make_unique<AmsCurrentToolData>();

    // Find the color swatch child
    data->color_swatch = lv_obj_find_by_name(widget, "color_swatch");
    if (!data->color_swatch) {
        spdlog::warn("[AmsCurrentTool] Could not find color_swatch child");
        return;
    }

    // Set initial color from current subject value
    lv_subject_t* color_subject = AmsState::instance().get_current_color_subject();
    if (color_subject) {
        int color_int = lv_subject_get_int(color_subject);
        lv_color_t color = lv_color_hex(static_cast<uint32_t>(color_int));
        lv_obj_set_style_bg_color(data->color_swatch, color, 0);

        // Observe future color changes using ObserverGuard for RAII cleanup [L020]
        data->color_observer = ObserverGuard(color_subject, on_color_changed, widget);
    }

    // Register cleanup callback
    lv_obj_add_event_cb(widget, on_delete, LV_EVENT_DELETE, nullptr);

    // Store data
    s_registry[widget] = std::move(data);

    spdlog::debug("[AmsCurrentTool] Widget initialized");
}

} // namespace

// Module initialization - call once during app startup
void ui_ams_current_tool_init() {
    // Register click callback for XML event_cb [L007]
    lv_xml_register_event_cb(nullptr, "on_ams_current_tool_clicked", on_clicked);

    spdlog::info("[AmsCurrentTool] Callbacks registered");
}

// Called after lv_xml_create() for ams_current_tool components
// Must be called manually since LVGL doesn't have automatic post-create hooks
void ui_ams_current_tool_setup(lv_obj_t* widget) {
    on_widget_created(widget);
}
