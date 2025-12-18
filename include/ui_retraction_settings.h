// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_panel_base.h"

#include "lvgl.h"
#include "moonraker_client.h"
#include "printer_state.h"

/**
 * @file ui_retraction_settings.h
 * @brief Firmware retraction settings overlay panel
 *
 * Configures Klipper firmware_retraction module parameters for G10/G11 retraction.
 * Provides sliders for retract length, speed, unretract extra, and unretract speed.
 *
 * ## Features
 * - Enable/disable firmware retraction
 * - Retract length (0-6mm, 0.1mm steps)
 * - Retract speed (10-80 mm/s)
 * - Unretract extra length (0-1mm, 0.1mm steps)
 * - Unretract speed (10-60 mm/s)
 *
 * ## Klipper G-codes
 * - SET_RETRACTION RETRACT_LENGTH=X RETRACT_SPEED=Y UNRETRACT_EXTRA_LENGTH=Z UNRETRACT_SPEED=W
 *
 * Values are stored in PrinterState subjects and synced from Moonraker subscription.
 */
class RetractionSettingsOverlay : public PanelBase {
  public:
    /**
     * @brief Construct RetractionSettingsOverlay
     * @param printer_state Reference to global printer state
     * @param client Pointer to MoonrakerClient for sending G-code
     */
    RetractionSettingsOverlay(PrinterState& printer_state, MoonrakerClient* client);
    ~RetractionSettingsOverlay() override;

    // PanelBase interface
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen) override;
    [[nodiscard]] const char* get_name() const override {
        return "RetractionSettings";
    }
    [[nodiscard]] const char* get_xml_component_name() const override {
        return "retraction_settings_overlay";
    }
    void init_subjects() override;

    // Lifecycle hooks
    void on_activate() override;
    void on_deactivate() override;

  private:
    /**
     * @brief Send SET_RETRACTION G-code with current values
     */
    void send_retraction_settings();

    /**
     * @brief Update display labels from current slider values
     */
    void update_display_labels();

    /**
     * @brief Sync UI sliders from PrinterState subjects
     */
    void sync_from_printer_state();

    // Event handlers
    static void on_enabled_changed(lv_event_t* e);
    static void on_retract_length_changed(lv_event_t* e);
    static void on_retract_speed_changed(lv_event_t* e);
    static void on_unretract_extra_changed(lv_event_t* e);
    static void on_unretract_speed_changed(lv_event_t* e);

    // Widget references
    lv_obj_t* enable_switch_ = nullptr;
    lv_obj_t* retract_length_slider_ = nullptr;
    lv_obj_t* retract_speed_slider_ = nullptr;
    lv_obj_t* unretract_extra_slider_ = nullptr;
    lv_obj_t* unretract_speed_slider_ = nullptr;

    // Display label subjects
    lv_subject_t retract_length_display_;
    lv_subject_t retract_speed_display_;
    lv_subject_t unretract_extra_display_;
    lv_subject_t unretract_speed_display_;

    // Static buffers for subject strings
    char retract_length_buf_[16];
    char retract_speed_buf_[16];
    char unretract_extra_buf_[16];
    char unretract_speed_buf_[16];

    // Moonraker client for G-code
    MoonrakerClient* client_ = nullptr;

    // Debounce - don't send G-code while syncing from printer state
    bool syncing_from_state_ = false;
};

// Global accessor
RetractionSettingsOverlay& get_global_retraction_settings();
void init_global_retraction_settings(PrinterState& printer_state, MoonrakerClient* client);
