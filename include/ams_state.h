// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ams_backend.h"
#include "ams_types.h"
#include "lvgl/lvgl.h"

#include <memory>
#include <mutex>

// Forward declarations
class PrinterCapabilities;
class MoonrakerAPI;
class MoonrakerClient;

/**
 * @file ams_state.h
 * @brief LVGL reactive state management for AMS UI binding
 *
 * Provides LVGL subjects that automatically update bound XML widgets
 * when AMS state changes. Bridges the AmsBackend to the UI layer.
 *
 * Usage:
 * 1. Call init_subjects() BEFORE creating XML components
 * 2. Call set_backend() to connect to an AMS backend
 * 3. Subjects auto-update when backend emits events
 *
 * Thread Safety:
 * All public methods are thread-safe. Subject updates are posted
 * to LVGL's thread via lv_async_call when called from background threads.
 */
class AmsState {
  public:
    /**
     * @brief Maximum number of slots supported for per-slot subjects
     *
     * Per-slot subjects (color, status) are allocated statically.
     * Systems with more slots will only have subjects for the first MAX_SLOTS.
     */
    static constexpr int MAX_SLOTS = 16;

    /**
     * @brief Get the singleton instance
     * @return Reference to the global AmsState instance
     */
    static AmsState& instance();

    // Non-copyable, non-movable singleton
    AmsState(const AmsState&) = delete;
    AmsState& operator=(const AmsState&) = delete;

    /**
     * @brief Initialize all LVGL subjects
     *
     * MUST be called BEFORE creating XML components that bind to these subjects.
     * Can be called multiple times safely - subsequent calls are ignored.
     *
     * @param register_xml If true, registers subjects with LVGL XML system (default).
     *                     Set to false in tests to avoid XML observer creation.
     */
    void init_subjects(bool register_xml = true);

    /**
     * @brief Reset initialization state for testing
     *
     * FOR TESTING ONLY. Clears the initialization flag so init_subjects()
     * can be called again after lv_init() creates a new LVGL context.
     */
    void reset_for_testing();

    /**
     * @brief Initialize AMS backend from detected printer capabilities
     *
     * Called after Moonraker discovery completes. If the printer has an MMU system
     * (AFC/Box Turtle, Happy Hare, etc.), creates and starts the appropriate backend.
     * Does nothing if no MMU is detected or if already in mock mode.
     *
     * @param caps Detected printer capabilities
     * @param api MoonrakerAPI instance for making API calls
     * @param client MoonrakerClient instance for WebSocket communication
     */
    void init_backend_from_capabilities(const PrinterCapabilities& caps, MoonrakerAPI* api,
                                        MoonrakerClient* client);

    /**
     * @brief Set the AMS backend
     *
     * Connects to the backend and starts receiving state updates.
     * Automatically registers event callback to sync state.
     *
     * @param backend Backend instance (ownership transferred)
     */
    void set_backend(std::unique_ptr<AmsBackend> backend);

    /**
     * @brief Get the current backend
     * @return Pointer to backend (may be nullptr)
     */
    [[nodiscard]] AmsBackend* get_backend() const;

    /**
     * @brief Check if AMS is available
     * @return true if backend is set and AMS type is not NONE
     */
    [[nodiscard]] bool is_available() const;

    // ========================================================================
    // System-level Subject Accessors
    // ========================================================================

    /**
     * @brief Get AMS type subject
     * @return Subject holding AmsType enum as int (0=none, 1=happy_hare, 2=afc)
     */
    lv_subject_t* get_ams_type_subject() {
        return &ams_type_;
    }

    /**
     * @brief Get current action subject
     * @return Subject holding AmsAction enum as int
     */
    lv_subject_t* get_ams_action_subject() {
        return &ams_action_;
    }

    /**
     * @brief Get action detail string subject
     * @return Subject holding current operation description
     */
    lv_subject_t* get_ams_action_detail_subject() {
        return &ams_action_detail_;
    }

    /**
     * @brief Get system name subject
     * @return Subject holding AMS system display name (e.g., "Happy Hare", "AFC")
     */
    lv_subject_t* get_ams_system_name_subject() {
        return &ams_system_name_;
    }

    /**
     * @brief Get current slot subject
     * @return Subject holding current slot index (-1 if none)
     */
    lv_subject_t* get_current_slot_subject() {
        return &current_slot_;
    }

    /**
     * @brief Get current tool subject
     * @return Subject holding current tool index (-1 if none)
     */
    lv_subject_t* get_current_tool_subject() {
        return &current_tool_;
    }

    /**
     * @brief Get filament loaded subject
     * @return Subject holding 0 (not loaded) or 1 (loaded)
     */
    lv_subject_t* get_filament_loaded_subject() {
        return &filament_loaded_;
    }

    /**
     * @brief Get bypass active subject
     *
     * Bypass mode allows external spool to feed directly to toolhead,
     * bypassing the MMU/hub system.
     *
     * @return Subject holding 0 (bypass inactive) or 1 (bypass active)
     */
    lv_subject_t* get_bypass_active_subject() {
        return &bypass_active_;
    }

    /**
     * @brief Get slot count subject
     * @return Subject holding total number of slots
     */
    lv_subject_t* get_slot_count_subject() {
        return &slot_count_;
    }

    /**
     * @brief Get slots version subject
     *
     * Incremented whenever slot data changes. UI can observe this
     * to know when to refresh slot displays.
     *
     * @return Subject holding version counter
     */
    lv_subject_t* get_slots_version_subject() {
        return &slots_version_;
    }

    // ========================================================================
    // Filament Path Visualization Subjects
    // ========================================================================

    /**
     * @brief Get path topology subject
     * @return Subject holding PathTopology enum as int (0=linear, 1=hub)
     */
    lv_subject_t* get_path_topology_subject() {
        return &path_topology_;
    }

    /**
     * @brief Get path active slot subject
     * @return Subject holding slot index whose path is being shown (-1=none)
     */
    lv_subject_t* get_path_active_slot_subject() {
        return &path_active_slot_;
    }

    /**
     * @brief Get path filament segment subject
     *
     * Indicates where the filament currently is along the path.
     *
     * @return Subject holding PathSegment enum as int
     */
    lv_subject_t* get_path_filament_segment_subject() {
        return &path_filament_segment_;
    }

    /**
     * @brief Get path error segment subject
     *
     * Indicates which segment has an error (for highlighting).
     *
     * @return Subject holding PathSegment enum as int (NONE if no error)
     */
    lv_subject_t* get_path_error_segment_subject() {
        return &path_error_segment_;
    }

    /**
     * @brief Get path animation progress subject
     *
     * Used for load/unload animations.
     *
     * @return Subject holding progress 0-100
     */
    lv_subject_t* get_path_anim_progress_subject() {
        return &path_anim_progress_;
    }

    // ========================================================================
    // Dryer Subject Accessors (for AMS systems with integrated drying)
    // ========================================================================

    /**
     * @brief Get dryer supported subject
     * @return Subject holding 1 if dryer is available, 0 otherwise
     */
    lv_subject_t* get_dryer_supported_subject() {
        return &dryer_supported_;
    }

    /**
     * @brief Get dryer active subject
     * @return Subject holding 1 if currently drying, 0 otherwise
     */
    lv_subject_t* get_dryer_active_subject() {
        return &dryer_active_;
    }

    /**
     * @brief Get dryer current temperature subject
     * @return Subject holding current temp in degrees C (integer)
     */
    lv_subject_t* get_dryer_current_temp_subject() {
        return &dryer_current_temp_;
    }

    /**
     * @brief Get dryer target temperature subject
     * @return Subject holding target temp in degrees C (integer, 0 = off)
     */
    lv_subject_t* get_dryer_target_temp_subject() {
        return &dryer_target_temp_;
    }

    /**
     * @brief Get dryer remaining minutes subject
     * @return Subject holding minutes remaining
     */
    lv_subject_t* get_dryer_remaining_min_subject() {
        return &dryer_remaining_min_;
    }

    /**
     * @brief Get dryer progress percentage subject
     * @return Subject holding 0-100 progress, or -1 if not drying
     */
    lv_subject_t* get_dryer_progress_pct_subject() {
        return &dryer_progress_pct_;
    }

    /**
     * @brief Get dryer current temperature text subject
     * @return Subject holding formatted temp string (e.g., "45C")
     */
    lv_subject_t* get_dryer_current_temp_text_subject() {
        return &dryer_current_temp_text_;
    }

    /**
     * @brief Get dryer target temperature text subject
     * @return Subject holding formatted temp string (e.g., "55C" or "---")
     */
    lv_subject_t* get_dryer_target_temp_text_subject() {
        return &dryer_target_temp_text_;
    }

    /**
     * @brief Get dryer time remaining text subject
     * @return Subject holding formatted time string (e.g., "2:30 left" or "")
     */
    lv_subject_t* get_dryer_time_text_subject() {
        return &dryer_time_text_;
    }

    // ========================================================================
    // Per-Slot Subject Accessors
    // ========================================================================

    /**
     * @brief Get slot color subject for a specific slot
     *
     * Holds 0xRRGGBB color value for UI display.
     *
     * @param slot_index Slot index (0 to MAX_SLOTS-1)
     * @return Subject pointer or nullptr if out of range
     */
    [[nodiscard]] lv_subject_t* get_slot_color_subject(int slot_index);

    /**
     * @brief Get slot status subject for a specific slot
     *
     * Holds SlotStatus enum as int.
     *
     * @param slot_index Slot index (0 to MAX_SLOTS-1)
     * @return Subject pointer or nullptr if out of range
     */
    [[nodiscard]] lv_subject_t* get_slot_status_subject(int slot_index);

    // ========================================================================
    // Direct State Update (called by backend event handler)
    // ========================================================================

    /**
     * @brief Update state from backend system info
     *
     * Called internally when backend emits STATE_CHANGED event.
     * Updates all subjects from the current backend state.
     */
    void sync_from_backend();

    /**
     * @brief Update a single slot's subjects
     *
     * Called when backend emits SLOT_CHANGED event.
     *
     * @param slot_index Slot that changed
     */
    void update_slot(int slot_index);

    /**
     * @brief Update dryer subjects from backend dryer info
     *
     * Called when backend reports dryer state changes.
     * Updates all dryer-related subjects for UI binding.
     */
    void sync_dryer_from_backend();

  private:
    AmsState();
    ~AmsState();

    /**
     * @brief Handle backend event callback
     * @param event Event name
     * @param data Event data
     */
    void on_backend_event(const std::string& event, const std::string& data);

    /**
     * @brief Bump the slots version counter
     */
    void bump_slots_version();

    /**
     * @brief Initialize a Klipper-based MMU backend (Happy Hare, AFC)
     *
     * Called when a Klipper object-based MMU system is detected.
     *
     * @param caps Detected printer capabilities
     * @param api MoonrakerAPI instance
     * @param client MoonrakerClient instance
     */
    void init_klipper_mmu_backend(const PrinterCapabilities& caps, MoonrakerAPI* api,
                                  MoonrakerClient* client);

    /**
     * @brief Probe for ValgACE via REST endpoint
     *
     * Makes an async REST call to /server/ace/info. If successful,
     * creates ValgACE backend via lv_async_call to maintain thread safety.
     *
     * @param api MoonrakerAPI instance for REST calls
     * @param client MoonrakerClient instance for the backend
     */
    void probe_valgace(MoonrakerAPI* api, MoonrakerClient* client);

    /**
     * @brief Create and start ValgACE backend
     *
     * Called on main thread after successful ValgACE probe.
     * Must be called from LVGL thread context.
     *
     * @param api MoonrakerAPI instance
     * @param client MoonrakerClient instance
     */
    void create_valgace_backend(MoonrakerAPI* api, MoonrakerClient* client);

    mutable std::recursive_mutex mutex_;
    std::unique_ptr<AmsBackend> backend_;
    bool initialized_ = false;

    // System-level subjects
    lv_subject_t ams_type_;
    lv_subject_t ams_action_;
    lv_subject_t current_slot_;
    lv_subject_t current_tool_;
    lv_subject_t filament_loaded_;
    lv_subject_t bypass_active_;
    lv_subject_t slot_count_;
    lv_subject_t slots_version_;

    // String subjects (need buffers)
    lv_subject_t ams_action_detail_;
    char action_detail_buf_[64];
    lv_subject_t ams_system_name_;
    char system_name_buf_[32];

    // Filament path visualization subjects
    lv_subject_t path_topology_;
    lv_subject_t path_active_slot_;
    lv_subject_t path_filament_segment_;
    lv_subject_t path_error_segment_;
    lv_subject_t path_anim_progress_;

    // Dryer subjects (for AMS systems with integrated drying)
    lv_subject_t dryer_supported_;
    lv_subject_t dryer_active_;
    lv_subject_t dryer_current_temp_;
    lv_subject_t dryer_target_temp_;
    lv_subject_t dryer_remaining_min_;
    lv_subject_t dryer_progress_pct_;

    // Dryer text subjects (need buffers)
    lv_subject_t dryer_current_temp_text_;
    char dryer_current_temp_text_buf_[16];
    lv_subject_t dryer_target_temp_text_;
    char dryer_target_temp_text_buf_[16];
    lv_subject_t dryer_time_text_;
    char dryer_time_text_buf_[32];

    // Per-slot subjects (color and status)
    lv_subject_t slot_colors_[MAX_SLOTS];
    lv_subject_t slot_statuses_[MAX_SLOTS];
};
