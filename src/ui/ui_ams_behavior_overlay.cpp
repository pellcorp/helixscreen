// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_behavior_overlay.cpp
 * @brief Implementation of AmsBehaviorOverlay
 */

#include "ui_ams_behavior_overlay.h"

#include "ui_event_safety.h"
#include "ui_nav_manager.h"

#include "ams_backend.h"
#include "ams_state.h"
#include "static_panel_registry.h"

#include <spdlog/spdlog.h>

#include <memory>

namespace helix::ui {

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

static std::unique_ptr<AmsBehaviorOverlay> g_ams_behavior_overlay;

AmsBehaviorOverlay& get_ams_behavior_overlay() {
    if (!g_ams_behavior_overlay) {
        g_ams_behavior_overlay = std::make_unique<AmsBehaviorOverlay>();
        StaticPanelRegistry::instance().register_destroy("AmsBehaviorOverlay",
                                                         []() { g_ams_behavior_overlay.reset(); });
    }
    return *g_ams_behavior_overlay;
}

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AmsBehaviorOverlay::AmsBehaviorOverlay() {
    spdlog::debug("[{}] Created", get_name());
}

AmsBehaviorOverlay::~AmsBehaviorOverlay() {
    if (subjects_initialized_ && lv_is_initialized()) {
        lv_subject_deinit(&supports_bypass_subject_);
        lv_subject_deinit(&bypass_active_subject_);
        lv_subject_deinit(&supports_auto_heat_subject_);
        lv_subject_deinit(&has_features_subject_);
    }
    spdlog::debug("[{}] Destroyed", get_name());
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void AmsBehaviorOverlay::init_subjects() {
    if (subjects_initialized_) {
        return;
    }

    // Initialize bypass support subject (0=not supported, 1=supported)
    lv_subject_init_int(&supports_bypass_subject_, 0);
    lv_xml_register_subject(nullptr, "ams_behavior_supports_bypass", &supports_bypass_subject_);

    // Initialize bypass active subject (0=inactive, 1=active)
    lv_subject_init_int(&bypass_active_subject_, 0);
    lv_xml_register_subject(nullptr, "ams_behavior_bypass_active", &bypass_active_subject_);

    // Initialize auto-heat support subject (0=not supported, 1=supported)
    lv_subject_init_int(&supports_auto_heat_subject_, 0);
    lv_xml_register_subject(nullptr, "ams_behavior_supports_auto_heat",
                            &supports_auto_heat_subject_);

    // Initialize has-features subject (0=no features, 1=has features)
    lv_subject_init_int(&has_features_subject_, 0);
    lv_xml_register_subject(nullptr, "ams_behavior_has_features", &has_features_subject_);

    subjects_initialized_ = true;
    spdlog::debug("[{}] Subjects initialized", get_name());
}

void AmsBehaviorOverlay::register_callbacks() {
    // Register bypass toggle callback
    lv_xml_register_event_cb(nullptr, "on_ams_behavior_bypass_toggled", on_bypass_toggled);

    spdlog::debug("[{}] Callbacks registered", get_name());
}

// ============================================================================
// UI CREATION
// ============================================================================

lv_obj_t* AmsBehaviorOverlay::create(lv_obj_t* parent) {
    if (overlay_) {
        spdlog::warn("[{}] create() called but overlay already exists", get_name());
        return overlay_;
    }

    spdlog::debug("[{}] Creating overlay...", get_name());

    // Create from XML component
    overlay_ = static_cast<lv_obj_t*>(lv_xml_create(parent, "ams_settings_behavior", nullptr));
    if (!overlay_) {
        spdlog::error("[{}] Failed to create overlay from XML", get_name());
        return nullptr;
    }

    // Find card widgets for programmatic visibility control
    bypass_card_ = lv_obj_find_by_name(overlay_, "bypass_card");
    auto_heat_card_ = lv_obj_find_by_name(overlay_, "auto_heat_card");
    no_features_card_ = lv_obj_find_by_name(overlay_, "no_features_card");

    // Initially hidden until show() pushes it
    lv_obj_add_flag(overlay_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[{}] Overlay created", get_name());
    return overlay_;
}

void AmsBehaviorOverlay::show(lv_obj_t* parent_screen) {
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

    // Update from backend
    refresh();

    // Register with NavigationManager for lifecycle callbacks
    NavigationManager::instance().register_overlay_instance(overlay_, this);

    // Push onto navigation stack
    ui_nav_push_overlay(overlay_);
}

void AmsBehaviorOverlay::refresh() {
    if (!overlay_) {
        return;
    }

    update_from_backend();
}

// ============================================================================
// BACKEND QUERIES
// ============================================================================

void AmsBehaviorOverlay::update_from_backend() {
    AmsBackend* backend = AmsState::instance().get_backend();
    if (!backend) {
        spdlog::warn("[{}] No backend available", get_name());
        // Show no features when no backend
        lv_subject_set_int(&supports_bypass_subject_, 0);
        lv_subject_set_int(&bypass_active_subject_, 0);
        lv_subject_set_int(&supports_auto_heat_subject_, 0);
        lv_subject_set_int(&has_features_subject_, 0);
        return;
    }

    // Query backend capabilities
    auto info = backend->get_system_info();
    bool supports_bypass = info.supports_bypass;
    bool bypass_active = backend->is_bypass_active();
    bool supports_auto_heat = backend->supports_auto_heat_on_load();

    spdlog::debug("[{}] Backend caps: bypass={}, bypass_active={}, auto_heat={}", get_name(),
                  supports_bypass, bypass_active, supports_auto_heat);

    // Update subjects - XML bindings handle visibility declaratively
    lv_subject_set_int(&supports_bypass_subject_, supports_bypass ? 1 : 0);
    lv_subject_set_int(&bypass_active_subject_, bypass_active ? 1 : 0);
    lv_subject_set_int(&supports_auto_heat_subject_, supports_auto_heat ? 1 : 0);

    // Update has_features subject - controls no_features_card and description visibility
    bool has_features = supports_bypass || supports_auto_heat;
    lv_subject_set_int(&has_features_subject_, has_features ? 1 : 0);
}

// ============================================================================
// STATIC CALLBACKS
// ============================================================================

void AmsBehaviorOverlay::on_bypass_toggled(lv_event_t* e) {
    LVGL_SAFE_EVENT_CB_BEGIN("[AmsBehaviorOverlay] on_bypass_toggled");

    auto* toggle = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (!toggle || !lv_obj_is_valid(toggle)) {
        spdlog::warn("[AmsBehaviorOverlay] Stale callback - toggle no longer valid");
    } else {
        bool is_checked = lv_obj_has_state(toggle, LV_STATE_CHECKED);

        spdlog::info("[AmsBehaviorOverlay] Bypass toggle: {}", is_checked ? "enabled" : "disabled");

        AmsBackend* backend = AmsState::instance().get_backend();
        if (backend) {
            AmsError result;
            if (is_checked) {
                result = backend->enable_bypass();
            } else {
                result = backend->disable_bypass();
            }

            if (result.success()) {
                spdlog::info("[AmsBehaviorOverlay] Bypass mode {}",
                             is_checked ? "enabled" : "disabled");
                // Update the active subject to reflect new state
                lv_subject_set_int(&get_ams_behavior_overlay().bypass_active_subject_,
                                   is_checked ? 1 : 0);
            } else {
                spdlog::error("[AmsBehaviorOverlay] Failed to {} bypass: {}",
                              is_checked ? "enable" : "disable", result.user_msg);
                // Revert the toggle state on failure
                if (is_checked) {
                    lv_obj_remove_state(toggle, LV_STATE_CHECKED);
                } else {
                    lv_obj_add_state(toggle, LV_STATE_CHECKED);
                }
            }
        } else {
            spdlog::error("[AmsBehaviorOverlay] No backend available for bypass toggle");
            // Revert the toggle state
            if (is_checked) {
                lv_obj_remove_state(toggle, LV_STATE_CHECKED);
            } else {
                lv_obj_add_state(toggle, LV_STATE_CHECKED);
            }
        }
    }

    LVGL_SAFE_EVENT_CB_END();
}

} // namespace helix::ui
