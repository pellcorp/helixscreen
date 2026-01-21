// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_behavior_overlay.h
 * @brief AMS Behavior sub-panel overlay
 *
 * This overlay allows users to configure AMS behavior settings including:
 * - Bypass mode toggle (feed filament directly to extruder)
 * - Auto-heat on load status (informational)
 *
 * @pattern Overlay (lazy init, singleton)
 * @threading Main thread only
 */

#pragma once

#include "overlay_base.h"

#include <lvgl/lvgl.h>

#include <memory>

// Forward declarations
class AmsBackend;

namespace helix::ui {

/**
 * @class AmsBehaviorOverlay
 * @brief Overlay for configuring AMS behavior settings
 *
 * This overlay provides toggles and information for AMS behavior configuration:
 * - Bypass mode: Allows feeding filament directly to extruder
 * - Auto-heat on load: Shows whether backend auto-heats based on material
 *
 * ## Usage:
 *
 * @code
 * auto& overlay = helix::ui::get_ams_behavior_overlay();
 * if (!overlay.are_subjects_initialized()) {
 *     overlay.init_subjects();
 *     overlay.register_callbacks();
 * }
 * overlay.show(parent_screen);
 * @endcode
 */
class AmsBehaviorOverlay : public OverlayBase {
  public:
    /**
     * @brief Default constructor
     */
    AmsBehaviorOverlay();

    /**
     * @brief Destructor
     */
    ~AmsBehaviorOverlay() override;

    // Non-copyable
    AmsBehaviorOverlay(const AmsBehaviorOverlay&) = delete;
    AmsBehaviorOverlay& operator=(const AmsBehaviorOverlay&) = delete;

    //
    // === OverlayBase Interface ===
    //

    /**
     * @brief Initialize subjects for reactive binding
     *
     * Registers subjects for:
     * - ams_behavior_supports_bypass: Whether bypass mode is supported (0/1)
     * - ams_behavior_bypass_active: Whether bypass is currently active (0/1)
     * - ams_behavior_supports_auto_heat: Whether auto-heat on load is supported (0/1)
     */
    void init_subjects() override;

    /**
     * @brief Register event callbacks with lv_xml system
     *
     * Registers callback for bypass toggle changes.
     */
    void register_callbacks() override;

    /**
     * @brief Create the overlay UI (called lazily)
     *
     * @param parent Parent widget to attach overlay to (usually screen)
     * @return Root object of overlay, or nullptr on failure
     */
    lv_obj_t* create(lv_obj_t* parent) override;

    /**
     * @brief Get human-readable overlay name
     * @return "Behavior"
     */
    const char* get_name() const override {
        return "Behavior";
    }

    //
    // === Public API ===
    //

    /**
     * @brief Show the overlay
     *
     * This method:
     * 1. Ensures overlay is created (lazy init)
     * 2. Queries backend for behavior capabilities
     * 3. Updates subject values based on backend state
     * 4. Pushes overlay onto navigation stack
     *
     * @param parent_screen The parent screen for overlay creation
     */
    void show(lv_obj_t* parent_screen);

    /**
     * @brief Refresh the behavior settings from backend
     *
     * Re-queries the backend and updates all subjects.
     */
    void refresh();

  private:
    //
    // === Internal Methods ===
    //

    /**
     * @brief Update subjects from backend state
     *
     * Queries backend for current capabilities and state:
     * - supports_bypass flag from system_info
     * - is_bypass_active() for current bypass state
     * - supports_auto_heat_on_load() for auto-heat capability
     * - has_features (any of the above supported)
     */
    void update_from_backend();

    //
    // === Static Callbacks ===
    //

    /**
     * @brief Callback for bypass toggle change
     *
     * Called when user toggles the bypass mode switch.
     * Calls backend enable_bypass() or disable_bypass() as appropriate.
     */
    static void on_bypass_toggled(lv_event_t* e);

    //
    // === State ===
    //

    /// Alias for overlay_root_ to match existing pattern
    lv_obj_t*& overlay_ = overlay_root_;

    /// Bypass card widget
    lv_obj_t* bypass_card_ = nullptr;

    /// Auto-heat card widget
    lv_obj_t* auto_heat_card_ = nullptr;

    /// No features card widget
    lv_obj_t* no_features_card_ = nullptr;

    /// Subject for bypass support (0=not supported, 1=supported)
    lv_subject_t supports_bypass_subject_;

    /// Subject for bypass active state (0=inactive, 1=active)
    lv_subject_t bypass_active_subject_;

    /// Subject for auto-heat support (0=not supported, 1=supported)
    lv_subject_t supports_auto_heat_subject_;

    /// Subject for whether any behavior features are available (0=none, 1=has features)
    lv_subject_t has_features_subject_;
};

/**
 * @brief Global instance accessor
 *
 * Creates the overlay on first access and registers it for cleanup
 * with StaticPanelRegistry.
 *
 * @return Reference to singleton AmsBehaviorOverlay
 */
AmsBehaviorOverlay& get_ams_behavior_overlay();

} // namespace helix::ui
