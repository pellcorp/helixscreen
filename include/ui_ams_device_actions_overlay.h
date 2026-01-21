// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file ui_ams_device_actions_overlay.h
 * @brief AMS Device Actions overlay for backend-specific controls
 *
 * This overlay displays device-specific actions organized by section.
 * Different AMS backends expose different capabilities:
 * - AFC: Calibration wizards, speed settings, lane maintenance
 * - Happy Hare: Servo calibration, encoder tests, gate checks
 * - ValgACE: Dryer presets, humidity readings
 *
 * Actions are dynamically queried from the backend via get_device_sections()
 * and get_device_actions(), allowing new features without UI changes.
 *
 * @pattern Overlay (lazy init, singleton)
 * @threading Main thread only
 */

#pragma once

#include "ams_types.h"
#include "overlay_base.h"

#include <lvgl/lvgl.h>

#include <memory>
#include <string>
#include <vector>

namespace helix::ui {

/**
 * @class AmsDeviceActionsOverlay
 * @brief Overlay for displaying and executing device-specific actions
 *
 * This overlay provides a dynamic interface for backend-specific features.
 * Actions are grouped by section (e.g., Calibration, Maintenance, Settings)
 * and rendered based on their type (button, toggle, slider, etc.).
 *
 * ## Usage:
 *
 * @code
 * auto& overlay = helix::ui::get_ams_device_actions_overlay();
 * if (!overlay.are_subjects_initialized()) {
 *     overlay.init_subjects();
 *     overlay.register_callbacks();
 * }
 * overlay.show(parent_screen);
 *
 * // Or to show only one section:
 * overlay.set_filter("calibration");
 * overlay.show(parent_screen);
 * @endcode
 */
class AmsDeviceActionsOverlay : public OverlayBase {
  public:
    /**
     * @brief Default constructor
     */
    AmsDeviceActionsOverlay();

    /**
     * @brief Destructor
     */
    ~AmsDeviceActionsOverlay() override;

    // Non-copyable
    AmsDeviceActionsOverlay(const AmsDeviceActionsOverlay&) = delete;
    AmsDeviceActionsOverlay& operator=(const AmsDeviceActionsOverlay&) = delete;

    //
    // === OverlayBase Interface ===
    //

    /**
     * @brief Initialize subjects for reactive binding
     *
     * Registers subjects for:
     * - ams_device_actions_status: Current status text
     */
    void init_subjects() override;

    /**
     * @brief Register event callbacks with lv_xml system
     *
     * Registers callbacks for action buttons and navigation.
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
     * @return "Device Actions"
     */
    const char* get_name() const override {
        return "Device Actions";
    }

    //
    // === Public API ===
    //

    /**
     * @brief Show the overlay
     *
     * This method:
     * 1. Ensures overlay is created (lazy init)
     * 2. Queries backend for device sections and actions
     * 3. Builds UI dynamically from backend data
     * 4. Pushes overlay onto navigation stack
     *
     * @param parent_screen The parent screen for overlay creation
     */
    void show(lv_obj_t* parent_screen);

    /**
     * @brief Refresh UI from backend
     *
     * Rebuilds the sections and actions from current backend state.
     * Call after backend state changes or filter updates.
     */
    void refresh();

    /**
     * @brief Set section filter
     *
     * When set, only actions from the specified section are displayed.
     * Pass empty string to show all sections.
     *
     * @param section_id Section ID to filter to (empty = show all)
     */
    void set_filter(const std::string& section_id);

  private:
    //
    // === Internal Methods ===
    //

    /**
     * @brief Create UI for a single section
     *
     * Creates a card with section header and all actions in that section.
     *
     * @param parent Container to add section to
     * @param section Section metadata
     */
    void create_section_ui(lv_obj_t* parent, const helix::printer::DeviceSection& section);

    /**
     * @brief Create control for a single action
     *
     * Creates the appropriate control based on action type:
     * - BUTTON: Action button
     * - TOGGLE: On/off switch
     * - SLIDER: Value slider
     * - DROPDOWN: Selection dropdown
     * - INFO: Read-only label
     *
     * @param parent Container to add control to
     * @param action Action metadata
     */
    void create_action_control(lv_obj_t* parent, const helix::printer::DeviceAction& action);

    /**
     * @brief Clear all section UI
     *
     * Removes all dynamically created section cards and resets state.
     */
    void clear_sections();

    //
    // === Static Callbacks ===
    //

    /**
     * @brief Callback for action button click
     *
     * Retrieves action ID from user_data and executes via backend.
     */
    static void on_action_clicked(lv_event_t* e);

    /**
     * @brief Callback for back button click
     *
     * Pops overlay from navigation stack.
     */
    static void on_back_clicked(lv_event_t* e);

    //
    // === State ===
    //

    /// Alias for overlay_root_ to match existing pattern
    lv_obj_t*& overlay_ = overlay_root_;

    /// Container for dynamically created sections
    lv_obj_t* sections_container_ = nullptr;

    /// Subject for status text display
    lv_subject_t status_subject_;

    /// Buffer for status text
    char status_buf_[128] = {};

    /// Section filter (empty = show all)
    std::string section_filter_;

    /// Cached sections from backend
    std::vector<helix::printer::DeviceSection> cached_sections_;

    /// Cached actions from backend
    std::vector<helix::printer::DeviceAction> cached_actions_;

    /// Action IDs for callback lookup (index stored in user_data)
    std::vector<std::string> action_ids_;
};

/**
 * @brief Global instance accessor
 *
 * Creates the overlay on first access and registers it for cleanup
 * with StaticPanelRegistry.
 *
 * @return Reference to singleton AmsDeviceActionsOverlay
 */
AmsDeviceActionsOverlay& get_ams_device_actions_overlay();

} // namespace helix::ui
