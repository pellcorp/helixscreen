// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "command_sequencer.h"
#include "gcode_file_modifier.h"
#include "gcode_ops_detector.h"
#include "moonraker_api.h"
#include "printer_state.h"

#include <lvgl.h>

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace helix::ui {

/**
 * @file ui_print_preparation_manager.h
 * @brief Manages pre-print operations and G-code modification
 *
 * Handles the print preparation workflow including:
 * - Scanning G-code files for embedded operations (bed leveling, QGL, etc.)
 * - Collecting user-selected pre-print options from checkboxes
 * - Building and executing pre-print operation sequences
 * - Modifying G-code to disable embedded operations when requested
 *
 * ## Usage:
 * ```cpp
 * PrintPreparationManager prep_manager;
 * prep_manager.set_dependencies(api, printer_state);
 * prep_manager.set_checkboxes(bed_cb, qgl_cb, z_tilt_cb, clean_cb, timelapse_cb);
 *
 * // When detail view opens:
 * prep_manager.scan_file_for_operations(filename, current_path);
 *
 * // When print button clicked:
 * prep_manager.start_print(filename, current_path, on_navigate_to_status);
 * ```
 */

/**
 * @brief Pre-print options read from UI checkboxes
 */
struct PrePrintOptions {
    bool bed_leveling = false;
    bool qgl = false;
    bool z_tilt = false;
    bool nozzle_clean = false;
    bool timelapse = false;
};

/**
 * @brief Callback for navigating to print status panel
 */
using NavigateToStatusCallback = std::function<void()>;

/**
 * @brief Callback for preparing state updates
 */
using PreparingCallback = std::function<void(const std::string& op_name, int step, int total)>;

/**
 * @brief Callback for preparing progress updates
 */
using PreparingProgressCallback = std::function<void(float progress)>;

/**
 * @brief Callback for print completion (success or failure)
 */
using PrintCompletionCallback = std::function<void(bool success, const std::string& error)>;

/**
 * @brief Manages print preparation workflow
 */
class PrintPreparationManager {
  public:
    PrintPreparationManager() = default;
    ~PrintPreparationManager();

    // Non-copyable, movable
    PrintPreparationManager(const PrintPreparationManager&) = delete;
    PrintPreparationManager& operator=(const PrintPreparationManager&) = delete;
    PrintPreparationManager(PrintPreparationManager&&) noexcept = default;
    PrintPreparationManager& operator=(PrintPreparationManager&&) noexcept = default;

    // === Setup ===

    /**
     * @brief Set API and printer state dependencies
     */
    void set_dependencies(MoonrakerAPI* api, PrinterState* printer_state);

    /**
     * @brief Set checkbox widget references for reading user selections
     *
     * @param bed_leveling Bed leveling checkbox (may be nullptr)
     * @param qgl QGL checkbox (may be nullptr)
     * @param z_tilt Z-tilt checkbox (may be nullptr)
     * @param nozzle_clean Nozzle clean checkbox (may be nullptr)
     * @param timelapse Timelapse checkbox (may be nullptr)
     */
    void set_checkboxes(lv_obj_t* bed_leveling, lv_obj_t* qgl, lv_obj_t* z_tilt,
                        lv_obj_t* nozzle_clean, lv_obj_t* timelapse);

    // === G-code Scanning ===

    /**
     * @brief Scan a G-code file for embedded operations (async)
     *
     * Downloads file content and scans for operations like bed leveling, QGL, etc.
     * Result is cached until a different file is scanned.
     *
     * @param filename File name (relative to gcodes root)
     * @param current_path Current directory path (empty = root)
     */
    void scan_file_for_operations(const std::string& filename, const std::string& current_path);

    /**
     * @brief Clear cached scan result
     */
    void clear_scan_cache();

    /**
     * @brief Check if scan result is available for a file
     */
    [[nodiscard]] bool has_scan_result_for(const std::string& filename) const;

    /**
     * @brief Get cached scan result (if available)
     */
    [[nodiscard]] const std::optional<gcode::ScanResult>& get_scan_result() const {
        return cached_scan_result_;
    }

    // === Print Execution ===

    /**
     * @brief Read pre-print options from checkbox states
     */
    [[nodiscard]] PrePrintOptions read_options_from_checkboxes() const;

    /**
     * @brief Start print with optional pre-print operations
     *
     * Handles the full workflow:
     * 1. Read checkbox states for pre-print options
     * 2. Check if user disabled operations embedded in G-code
     * 3. If so, modify file and print modified version
     * 4. Otherwise, execute pre-print sequence (if any) then print
     *
     * @param filename File to print
     * @param current_path Current directory path
     * @param on_navigate_to_status Callback to navigate to print status panel
     * @param on_preparing Optional callback for preparing state updates
     * @param on_progress Optional callback for preparing progress updates
     * @param on_completion Optional callback for print completion
     */
    void start_print(const std::string& filename, const std::string& current_path,
                     NavigateToStatusCallback on_navigate_to_status,
                     PreparingCallback on_preparing = nullptr,
                     PreparingProgressCallback on_progress = nullptr,
                     PrintCompletionCallback on_completion = nullptr);

    /**
     * @brief Check if a pre-print sequence is currently running
     */
    [[nodiscard]] bool is_preparing() const { return pre_print_sequencer_ != nullptr; }

    /**
     * @brief Cancel any running pre-print sequence
     */
    void cancel_preparation();

  private:
    // === Dependencies ===
    MoonrakerAPI* api_ = nullptr;
    PrinterState* printer_state_ = nullptr;

    // === Checkbox References ===
    lv_obj_t* bed_leveling_checkbox_ = nullptr;
    lv_obj_t* qgl_checkbox_ = nullptr;
    lv_obj_t* z_tilt_checkbox_ = nullptr;
    lv_obj_t* nozzle_clean_checkbox_ = nullptr;
    lv_obj_t* timelapse_checkbox_ = nullptr;

    // === Scan Cache ===
    std::optional<gcode::ScanResult> cached_scan_result_;
    std::string cached_scan_filename_;

    // === Command Sequencer ===
    std::unique_ptr<gcode::CommandSequencer> pre_print_sequencer_;

    // === Internal Methods ===

    /**
     * @brief Collect operations that user wants to disable
     *
     * Compares checkbox states against cached scan result to identify
     * operations that are embedded in the file but disabled by user.
     */
    [[nodiscard]] std::vector<gcode::OperationType> collect_ops_to_disable() const;

    /**
     * @brief Download, modify, and print a G-code file
     *
     * Used when user disabled an operation that's embedded in the G-code.
     *
     * @param file_path Full path to file relative to gcodes root
     * @param ops_to_disable Operations to comment out in the file
     * @param on_navigate_to_status Callback to navigate to print status panel
     */
    void modify_and_print(const std::string& file_path,
                          const std::vector<gcode::OperationType>& ops_to_disable,
                          NavigateToStatusCallback on_navigate_to_status);

    /**
     * @brief Execute pre-print sequence then start print
     */
    void execute_pre_print_sequence(const std::string& filename, const PrePrintOptions& options,
                                    NavigateToStatusCallback on_navigate_to_status,
                                    PreparingCallback on_preparing,
                                    PreparingProgressCallback on_progress,
                                    PrintCompletionCallback on_completion);

    /**
     * @brief Start print directly (no pre-print operations)
     */
    void start_print_directly(const std::string& filename,
                              NavigateToStatusCallback on_navigate_to_status,
                              PrintCompletionCallback on_completion);

    /**
     * @brief Helper to check if a checkbox is visible and unchecked
     */
    static bool is_option_disabled(lv_obj_t* checkbox);
};

} // namespace helix::ui
