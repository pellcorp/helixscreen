// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <lvgl.h>
#include <memory>
#include <string>

class MoonrakerClient;

/**
 * @file ui_panel_calibration_pid.h
 * @brief PID Tuning Calibration Panel
 *
 * Interactive calibration using Klipper's PID_CALIBRATE command.
 * Supports both extruder and heated bed PID tuning.
 *
 * ## Klipper Commands Used:
 * - `PID_CALIBRATE HEATER=extruder TARGET=<temp>` - Extruder tuning
 * - `PID_CALIBRATE HEATER=heater_bed TARGET=<temp>` - Bed tuning
 * - `SAVE_CONFIG` - Persist results (restarts Klipper)
 *
 * ## State Machine:
 * IDLE → CALIBRATING → SAVING → COMPLETE
 *                   ↘ ERROR
 *
 * ## Typical Duration:
 * - Extruder: 3-5 minutes
 * - Heated Bed: 5-10 minutes (larger thermal mass)
 */
class PIDCalibrationPanel {
  public:
    /**
     * @brief Calibration state machine states
     */
    enum class State {
        IDLE,        ///< Ready to start, heater selection shown
        CALIBRATING, ///< PID_CALIBRATE running, showing progress
        SAVING,      ///< SAVE_CONFIG running, Klipper restarting
        COMPLETE,    ///< Calibration successful, showing results
        ERROR        ///< Something went wrong
    };

    /**
     * @brief Which heater is being calibrated
     */
    enum class Heater { EXTRUDER, BED };

    PIDCalibrationPanel() = default;
    ~PIDCalibrationPanel() = default;

    /**
     * @brief Initialize subjects and XML event callbacks
     *
     * Call once at startup before any panel instances are created.
     * Registers the pid_cal_state subject and all XML event callbacks.
     */
    static void init_subjects();

    /**
     * @brief Setup the panel with event handlers
     *
     * @param panel Root panel object from lv_xml_create()
     * @param parent_screen Parent screen for overlay management
     * @param client MoonrakerClient for sending G-code commands
     */
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen, MoonrakerClient* client);

    /**
     * @brief Get current state
     */
    State get_state() const {
        return state_;
    }

    /**
     * @brief Update current temperature display during calibration
     *
     * Called from temperature update callbacks to show live temp.
     *
     * @param current Current temperature reading
     * @param target Target temperature
     */
    void update_temperature(float current, float target);

    /**
     * @brief Called when calibration completes with results
     *
     * @param success True if calibration succeeded
     * @param kp Proportional gain (only valid if success)
     * @param ki Integral gain (only valid if success)
     * @param kd Derivative gain (only valid if success)
     * @param error_message Error description (only valid if !success)
     */
    void on_calibration_result(bool success, float kp = 0, float ki = 0, float kd = 0,
                               const std::string& error_message = "");

  private:
    // Panel and client references
    lv_obj_t* panel_ = nullptr;
    lv_obj_t* parent_screen_ = nullptr;
    MoonrakerClient* client_ = nullptr;

    // State
    State state_ = State::IDLE;
    Heater selected_heater_ = Heater::EXTRUDER;
    int target_temp_ = 200; // Default for extruder

    // Temperature limits
    static constexpr int EXTRUDER_MIN_TEMP = 150;
    static constexpr int EXTRUDER_MAX_TEMP = 280;
    static constexpr int EXTRUDER_DEFAULT_TEMP = 200;
    static constexpr int BED_MIN_TEMP = 40;
    static constexpr int BED_MAX_TEMP = 110;
    static constexpr int BED_DEFAULT_TEMP = 60;

    // PID results
    float result_kp_ = 0;
    float result_ki_ = 0;
    float result_kd_ = 0;

    // String subjects and buffers for reactive text updates
    lv_subject_t subj_temp_display_;
    char buf_temp_display_[16];

    lv_subject_t subj_temp_hint_;
    char buf_temp_hint_[64];

    lv_subject_t subj_current_temp_display_;
    char buf_current_temp_display_[32];

    lv_subject_t subj_calibrating_heater_;
    char buf_calibrating_heater_[32];

    lv_subject_t subj_pid_kp_;
    char buf_pid_kp_[16];

    lv_subject_t subj_pid_ki_;
    char buf_pid_ki_[16];

    lv_subject_t subj_pid_kd_;
    char buf_pid_kd_[16];

    lv_subject_t subj_error_message_;
    char buf_error_message_[256];

    // Widget references (only for imperative updates like styling)
    lv_obj_t* btn_heater_extruder_ = nullptr;
    lv_obj_t* btn_heater_bed_ = nullptr;

    // State management
    void set_state(State new_state);

    // UI updates
    void update_heater_selection();
    void update_temp_display();
    void update_temp_hint();

    // G-code commands
    void send_pid_calibrate();
    void send_save_config();

    // Event handlers
    void handle_heater_extruder_clicked();
    void handle_heater_bed_clicked();
    void handle_temp_up();
    void handle_temp_down();
    void handle_start_clicked();
    void handle_abort_clicked();
    void handle_done_clicked();
    void handle_retry_clicked();

    // Static trampolines
    static void on_heater_extruder_clicked(lv_event_t* e);
    static void on_heater_bed_clicked(lv_event_t* e);
    static void on_temp_up(lv_event_t* e);
    static void on_temp_down(lv_event_t* e);
    static void on_start_clicked(lv_event_t* e);
    static void on_abort_clicked(lv_event_t* e);
    static void on_done_clicked(lv_event_t* e);
    static void on_retry_clicked(lv_event_t* e);
};

// Global instance accessor
PIDCalibrationPanel& get_global_pid_cal_panel();
