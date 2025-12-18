// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "spoolman_types.h"

#include <functional>
#include <lvgl.h>
#include <memory>
#include <string>
#include <vector>

class MoonrakerAPI;

namespace helix::ui {

/**
 * @file ui_ams_spoolman_picker.h
 * @brief Modal picker for assigning Spoolman spools to AMS slots
 *
 * Displays a scrollable list of available spools from Spoolman with
 * vendor, material, color, and weight information. Supports assigning
 * or unlinking spools from AMS slots.
 *
 * ## Usage:
 * @code
 * helix::ui::AmsSpoolmanPicker picker;
 * picker.set_completion_callback([](const PickerResult& result) {
 *     if (result.action == PickerAction::ASSIGN) {
 *         // Assign result.spool_id to result.slot_index
 *     }
 * });
 * picker.show_for_slot(parent, slot_index, current_spool_id, api);
 * @endcode
 */
class AmsSpoolmanPicker {
  public:
    enum class PickerAction {
        CANCELLED, ///< User closed picker without action
        ASSIGN,    ///< User selected a spool to assign
        UNLINK     ///< User requested to unlink current spool
    };

    struct PickerResult {
        PickerAction action = PickerAction::CANCELLED;
        int slot_index = -1;  ///< Slot the picker was opened for
        int spool_id = 0;     ///< Selected spool ID (if action == ASSIGN)
        SpoolInfo spool_info; ///< Full spool info (if action == ASSIGN)
    };

    using CompletionCallback = std::function<void(const PickerResult& result)>;

    AmsSpoolmanPicker();
    ~AmsSpoolmanPicker();

    // Non-copyable
    AmsSpoolmanPicker(const AmsSpoolmanPicker&) = delete;
    AmsSpoolmanPicker& operator=(const AmsSpoolmanPicker&) = delete;

    // Movable
    AmsSpoolmanPicker(AmsSpoolmanPicker&& other) noexcept;
    AmsSpoolmanPicker& operator=(AmsSpoolmanPicker&& other) noexcept;

    /**
     * @brief Show picker for a specific slot
     * @param parent Parent screen for the modal
     * @param slot_index Slot to assign spool to (0-based)
     * @param current_spool_id Current Spoolman ID for this slot (0 if none)
     * @param api MoonrakerAPI for fetching spools
     * @return true if picker was shown successfully
     */
    bool show_for_slot(lv_obj_t* parent, int slot_index, int current_spool_id, MoonrakerAPI* api);

    /**
     * @brief Hide the picker
     */
    void hide();

    /**
     * @brief Check if picker is currently visible
     */
    [[nodiscard]] bool is_visible() const {
        return picker_ != nullptr;
    }

    /**
     * @brief Set callback for when picker closes
     */
    void set_completion_callback(CompletionCallback callback);

  private:
    // === State ===
    lv_obj_t* picker_ = nullptr;
    lv_obj_t* parent_ = nullptr;
    int slot_index_ = -1;
    int current_spool_id_ = 0;
    MoonrakerAPI* api_ = nullptr;
    CompletionCallback completion_callback_;

    // === Cached spools for selection lookup ===
    std::vector<SpoolInfo> cached_spools_;

    // === Async callback guard [L012] ===
    std::shared_ptr<bool> callback_guard_;

    // === Subjects for XML binding ===
    lv_subject_t slot_indicator_subject_;
    lv_subject_t picker_state_subject_; ///< 0=LOADING, 1=EMPTY, 2=CONTENT
    char slot_indicator_buf_[48] = {0};
    bool subjects_initialized_ = false;

    // === Observer tracking for cleanup [L020] ===
    lv_observer_t* slot_indicator_observer_ = nullptr;

    // === Internal Methods ===
    void init_subjects();
    void populate_spools();

    // === Event Handlers ===
    void handle_close();
    void handle_unlink();
    void handle_spool_selected(int spool_id);

    // === Static Callback Registration ===
    static void register_callbacks();
    static bool callbacks_registered_;

    // === Static Callbacks ===
    static void on_close_cb(lv_event_t* e);
    static void on_unlink_cb(lv_event_t* e);
    static void on_spool_item_cb(lv_event_t* e);

    /**
     * @brief Find picker instance from event target
     */
    static AmsSpoolmanPicker* get_instance_from_event(lv_event_t* e);
};

} // namespace helix::ui
