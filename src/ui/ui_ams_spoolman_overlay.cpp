// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_spoolman_overlay.cpp
 * @brief Implementation of AmsSpoolmanOverlay
 */

#include "ui_ams_spoolman_overlay.h"

#include "ui_event_safety.h"
#include "ui_nav_manager.h"

#include "ams_state.h"
#include "moonraker_client.h"
#include "static_panel_registry.h"

#include <spdlog/spdlog.h>

#include <memory>

namespace helix::ui {

// Database keys for settings persistence
static constexpr const char* DB_NAMESPACE = "helix-screen";
static constexpr const char* DB_KEY_SYNC_ENABLED = "ams_spoolman_sync_enabled";
static constexpr const char* DB_KEY_REFRESH_INTERVAL = "ams_weight_refresh_interval";

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

static std::unique_ptr<AmsSpoolmanOverlay> g_ams_spoolman_overlay;

AmsSpoolmanOverlay& get_ams_spoolman_overlay() {
    if (!g_ams_spoolman_overlay) {
        g_ams_spoolman_overlay = std::make_unique<AmsSpoolmanOverlay>();
        StaticPanelRegistry::instance().register_destroy("AmsSpoolmanOverlay",
                                                         []() { g_ams_spoolman_overlay.reset(); });
    }
    return *g_ams_spoolman_overlay;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AmsSpoolmanOverlay::AmsSpoolmanOverlay() {
    spdlog::debug("[{}] Created", get_name());
}

AmsSpoolmanOverlay::~AmsSpoolmanOverlay() {
    if (subjects_initialized_ && lv_is_initialized()) {
        lv_subject_deinit(&sync_enabled_subject_);
        lv_subject_deinit(&refresh_interval_subject_);
    }
    spdlog::debug("[{}] Destroyed", get_name());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void AmsSpoolmanOverlay::init_subjects() {
    if (subjects_initialized_) {
        return;
    }

    // Initialize sync enabled subject (default: true/enabled)
    lv_subject_init_int(&sync_enabled_subject_, DEFAULT_SYNC_ENABLED ? 1 : 0);
    lv_xml_register_subject(nullptr, "ams_spoolman_sync_enabled", &sync_enabled_subject_);

    // Initialize refresh interval subject (default: 30 seconds)
    lv_subject_init_int(&refresh_interval_subject_, DEFAULT_REFRESH_INTERVAL_SECONDS);
    lv_xml_register_subject(nullptr, "ams_spoolman_refresh_interval", &refresh_interval_subject_);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void AmsSpoolmanOverlay::register_callbacks() {
    // Register sync toggle callback
    lv_xml_register_event_cb(nullptr, "on_ams_spoolman_sync_toggled", on_sync_toggled);

    // Register interval dropdown callback
    lv_xml_register_event_cb(nullptr, "on_ams_spoolman_interval_changed", on_interval_changed);

    spdlog::debug("[{}] Callbacks registered", get_name());
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* AmsSpoolmanOverlay::create(lv_obj_t* parent) {
    if (overlay_) {
        spdlog::warn("[{}] create() called but overlay already exists", get_name());
        return overlay_;
    }

    spdlog::debug("[{}] Creating overlay...", get_name());

    // Create from XML component
    overlay_ = static_cast<lv_obj_t*>(lv_xml_create(parent, "ams_settings_spoolman", nullptr));
    if (!overlay_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Find control widgets for programmatic access
    sync_toggle_ = lv_obj_find_by_name(overlay_, "sync_toggle");
    interval_dropdown_ = lv_obj_find_by_name(overlay_, "interval_dropdown");

    // Initially hidden until show() pushes it
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[{}] Overlay created", get_name());
    return overlay_;
}

void AmsSpoolmanOverlay::show(lv_obj_t* parent_screen) {
    spdlog::debug("[{}] show() called", get_name());

    parent_screen_ = parent_screen;

    // Ensure subjects and callbacks are initialized
    if (!subjects_initialized_) {
        init_subjects();
        register_callbacks();
    }

    // Lazy create overlay
    if (!overlay_ && parent_screen_) {
        create(parent_screen_);
    }

    if (!overlay_) {
        spdlog::error("[{}] Cannot show - overlay not created", get_name());
        return;
    }

    // Load settings from database
    load_from_database();

    // Update UI controls to match subject values
    update_ui_from_subjects();

    // Register with NavigationManager for lifecycle callbacks
    NavigationManager::instance().register_overlay_instance(overlay_, this);

    // Push onto navigation stack
    ui_nav_push_overlay(overlay_);
}

void AmsSpoolmanOverlay::refresh() {
    if (!overlay_) {
        return;
    }

    load_from_database();
    update_ui_from_subjects();
}

// ============================================================================
// DATABASE OPERATIONS
// ============================================================================

void AmsSpoolmanOverlay::load_from_database() {
    if (!client_) {
        spdlog::warn("[{}] No client available, using default values", get_name());
        return;
    }

    // Load sync enabled setting
    nlohmann::json params_sync = {{"namespace", DB_NAMESPACE}, {"key", DB_KEY_SYNC_ENABLED}};

    client_->send_jsonrpc(
        "server.database.get_item", params_sync,
        [this](const nlohmann::json& response) {
            bool enabled = DEFAULT_SYNC_ENABLED;
            if (response.contains("value")) {
                if (response["value"].is_boolean()) {
                    enabled = response["value"].get<bool>();
                } else if (response["value"].is_number()) {
                    enabled = response["value"].get<int>() != 0;
                }
            }
            lv_subject_set_int(&sync_enabled_subject_, enabled ? 1 : 0);
            spdlog::debug("[{}] Loaded sync_enabled={} from database", get_name(), enabled);

            // Update AmsState polling based on loaded setting
            if (enabled) {
                AmsState::instance().start_spoolman_polling();
            } else {
                AmsState::instance().stop_spoolman_polling();
            }
        },
        [this](const MoonrakerError& err) {
            spdlog::debug("[{}] Could not load sync_enabled (using default): {}", get_name(),
                          err.message);
            // Use default value
            lv_subject_set_int(&sync_enabled_subject_, DEFAULT_SYNC_ENABLED ? 1 : 0);
        },
        0,     // timeout_ms = default
        true); // silent = true (key may not exist on first run)

    // Load refresh interval setting
    nlohmann::json params_interval = {{"namespace", DB_NAMESPACE},
                                      {"key", DB_KEY_REFRESH_INTERVAL}};

    client_->send_jsonrpc(
        "server.database.get_item", params_interval,
        [this](const nlohmann::json& response) {
            int interval = DEFAULT_REFRESH_INTERVAL_SECONDS;
            if (response.contains("value") && response["value"].is_number()) {
                interval = response["value"].get<int>();
            }
            lv_subject_set_int(&refresh_interval_subject_, interval);
            spdlog::debug("[{}] Loaded refresh_interval={} from database", get_name(), interval);
        },
        [this](const MoonrakerError& err) {
            spdlog::debug("[{}] Could not load refresh_interval (using default): {}", get_name(),
                          err.message);
            // Use default value
            lv_subject_set_int(&refresh_interval_subject_, DEFAULT_REFRESH_INTERVAL_SECONDS);
        },
        0,     // timeout_ms = default
        true); // silent = true (key may not exist on first run)
}

void AmsSpoolmanOverlay::save_sync_enabled(bool enabled) {
    if (!client_) {
        spdlog::warn("[{}] No client available, cannot save setting", get_name());
        return;
    }

    nlohmann::json params = {
        {"namespace", DB_NAMESPACE}, {"key", DB_KEY_SYNC_ENABLED}, {"value", enabled}};

    client_->send_jsonrpc(
        "server.database.post_item", params,
        [this, enabled](const nlohmann::json&) {
            spdlog::info("[{}] Saved sync_enabled={} to database", get_name(), enabled);
        },
        [this](const MoonrakerError& err) {
            spdlog::error("[{}] Failed to save sync_enabled: {}", get_name(), err.message);
        });
}

void AmsSpoolmanOverlay::save_refresh_interval(int interval_seconds) {
    if (!client_) {
        spdlog::warn("[{}] No client available, cannot save setting", get_name());
        return;
    }

    nlohmann::json params = {
        {"namespace", DB_NAMESPACE}, {"key", DB_KEY_REFRESH_INTERVAL}, {"value", interval_seconds}};

    client_->send_jsonrpc(
        "server.database.post_item", params,
        [this, interval_seconds](const nlohmann::json&) {
            spdlog::info("[{}] Saved refresh_interval={} to database", get_name(),
                         interval_seconds);
        },
        [this](const MoonrakerError& err) {
            spdlog::error("[{}] Failed to save refresh_interval: {}", get_name(), err.message);
        });
}

// ============================================================================
// UTILITY METHODS
// ============================================================================

int AmsSpoolmanOverlay::dropdown_index_to_seconds(int index) {
    // Dropdown options: "30s", "1 min", "2 min", "5 min"
    switch (index) {
    case 0:
        return 30;
    case 1:
        return 60;
    case 2:
        return 120;
    case 3:
        return 300;
    default:
        return 30;
    }
}

int AmsSpoolmanOverlay::seconds_to_dropdown_index(int seconds) {
    switch (seconds) {
    case 30:
        return 0;
    case 60:
        return 1;
    case 120:
        return 2;
    case 300:
        return 3;
    default:
        return 0; // Default to 30s
    }
}

void AmsSpoolmanOverlay::update_ui_from_subjects() {
    // Update dropdown to match current interval
    if (interval_dropdown_) {
        int interval_seconds = lv_subject_get_int(&refresh_interval_subject_);
        int dropdown_index = seconds_to_dropdown_index(interval_seconds);
        lv_dropdown_set_selected(interval_dropdown_, static_cast<uint32_t>(dropdown_index));
    }

    // Toggle state is handled by subject binding in XML
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void AmsSpoolmanOverlay::on_sync_toggled(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AmsSpoolmanOverlay] on_sync_toggled");

    auto* toggle = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (!toggle || !lv_obj_is_valid(toggle)) {
        spdlog::warn("[AmsSpoolmanOverlay] Stale callback - toggle no longer valid");
    } else {
        bool is_checked = lv_obj_has_state(toggle, LV_STATE_CHECKED);

        spdlog::info("[AmsSpoolmanOverlay] Sync toggle: {}", is_checked ? "enabled" : "disabled");

        // Update subject
        auto& overlay = get_ams_spoolman_overlay();
        lv_subject_set_int(&overlay.sync_enabled_subject_, is_checked ? 1 : 0);

        // Save to database
        overlay.save_sync_enabled(is_checked);

        // Update AmsState polling
        if (is_checked) {
            AmsState::instance().start_spoolman_polling();
        } else {
            AmsState::instance().stop_spoolman_polling();
        }
    }

    LVGL_SAFE_EVENT_CB_END();
}

void AmsSpoolmanOverlay::on_interval_changed(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AmsSpoolmanOverlay] on_interval_changed");

    auto* dropdown = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (!dropdown || !lv_obj_is_valid(dropdown)) {
        spdlog::warn("[AmsSpoolmanOverlay] Stale callback - dropdown no longer valid");
    } else {
        int selected = static_cast<int>(lv_dropdown_get_selected(dropdown));
        int interval_seconds = dropdown_index_to_seconds(selected);

        spdlog::info("[AmsSpoolmanOverlay] Interval changed: {}s", interval_seconds);

        // Update subject
        auto& overlay = get_ams_spoolman_overlay();
        lv_subject_set_int(&overlay.refresh_interval_subject_, interval_seconds);

        // Save to database
        overlay.save_refresh_interval(interval_seconds);

        // Note: The actual polling interval in AmsState is currently fixed at 30s.
        // This setting is stored for future use when configurable polling is implemented.
        // For now, we just persist the user's preference.
    }

    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::ui
