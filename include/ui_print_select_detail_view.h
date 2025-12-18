// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_print_preparation_manager.h"

#include <lvgl.h>

#include <functional>
#include <string>

// Forward declarations
class MoonrakerAPI;
class PrinterState;

namespace helix::ui {

/**
 * @file ui_print_select_detail_view.h
 * @brief Detail view overlay manager for print selection panel
 *
 * Handles the file detail overlay that appears when a file is selected,
 * including:
 * - Creating and positioning the detail view widget
 * - Showing/hiding with nav system integration
 * - Delete confirmation modal management
 * - Filament type dropdown synchronization
 *
 * ## Usage:
 * @code
 * PrintSelectDetailView detail_view;
 * detail_view.create(parent_screen);
 * detail_view.set_prep_manager(prep_manager);
 * detail_view.set_on_delete_confirmed([this]() { delete_file(); });
 *
 * // When file selected:
 * detail_view.show(filename, current_path, filament_type);
 *
 * // When back button clicked:
 * detail_view.hide();
 * @endcode
 */

/**
 * @brief Callback when delete is confirmed
 */
using DeleteConfirmedCallback = std::function<void()>;

/**
 * @brief Detail view overlay manager
 */
class PrintSelectDetailView {
  public:
    PrintSelectDetailView() = default;
    ~PrintSelectDetailView();

    // Non-copyable, non-movable (owns LVGL widgets with external references)
    PrintSelectDetailView(const PrintSelectDetailView&) = delete;
    PrintSelectDetailView& operator=(const PrintSelectDetailView&) = delete;
    PrintSelectDetailView(PrintSelectDetailView&&) = delete;
    PrintSelectDetailView& operator=(PrintSelectDetailView&&) = delete;

    // === Setup ===

    /**
     * @brief Create the detail view widget
     *
     * Creates the print_file_detail XML component and configures it.
     * Must be called before show().
     *
     * @param parent_screen Screen to create detail view on
     * @return true if created successfully
     */
    bool create(lv_obj_t* parent_screen);

    /**
     * @brief Set dependencies for print preparation
     *
     * @param api MoonrakerAPI for file operations
     * @param printer_state PrinterState for capability detection
     */
    void set_dependencies(MoonrakerAPI* api, PrinterState* printer_state);

    /**
     * @brief Set callback for delete confirmation
     */
    void set_on_delete_confirmed(DeleteConfirmedCallback callback) {
        on_delete_confirmed_ = std::move(callback);
    }

    /**
     * @brief Set the visible subject for XML binding
     *
     * The subject should be initialized to 0 (hidden).
     */
    void set_visible_subject(lv_subject_t* subject) { visible_subject_ = subject; }

    // === Visibility ===

    /**
     * @brief Show the detail view overlay
     *
     * Pushes overlay via nav system and triggers G-code scanning.
     *
     * @param filename Selected filename (for G-code scanning)
     * @param current_path Current directory path
     * @param filament_type Filament type from metadata (for dropdown default)
     */
    void show(const std::string& filename, const std::string& current_path,
              const std::string& filament_type);

    /**
     * @brief Hide the detail view overlay
     *
     * Uses nav system to properly hide with backdrop management.
     */
    void hide();

    /**
     * @brief Check if detail view is currently visible
     */
    [[nodiscard]] bool is_visible() const;

    // === Delete Confirmation ===

    /**
     * @brief Show delete confirmation dialog
     *
     * @param filename Filename to display in confirmation message
     */
    void show_delete_confirmation(const std::string& filename);

    /**
     * @brief Hide delete confirmation dialog
     */
    void hide_delete_confirmation();

    // === Widget Access ===

    /**
     * @brief Get the detail view widget
     */
    [[nodiscard]] lv_obj_t* get_widget() const { return detail_view_widget_; }

    /**
     * @brief Get the print button (for enable/disable state)
     */
    [[nodiscard]] lv_obj_t* get_print_button() const { return print_button_; }

    /**
     * @brief Get the print preparation manager
     */
    [[nodiscard]] PrintPreparationManager* get_prep_manager() const { return prep_manager_.get(); }

    // === Checkbox Access (for prep manager setup) ===

    [[nodiscard]] lv_obj_t* get_bed_leveling_checkbox() const { return bed_leveling_checkbox_; }
    [[nodiscard]] lv_obj_t* get_qgl_checkbox() const { return qgl_checkbox_; }
    [[nodiscard]] lv_obj_t* get_z_tilt_checkbox() const { return z_tilt_checkbox_; }
    [[nodiscard]] lv_obj_t* get_nozzle_clean_checkbox() const { return nozzle_clean_checkbox_; }
    [[nodiscard]] lv_obj_t* get_timelapse_checkbox() const { return timelapse_checkbox_; }

    // === Resize Handling ===

    /**
     * @brief Handle resize event - update responsive padding
     *
     * @param parent_screen Parent screen for height calculation
     */
    void handle_resize(lv_obj_t* parent_screen);

  private:
    // === Dependencies ===
    MoonrakerAPI* api_ = nullptr;
    PrinterState* printer_state_ = nullptr;
    lv_subject_t* visible_subject_ = nullptr;

    // === Widget References ===
    lv_obj_t* parent_screen_ = nullptr;
    lv_obj_t* detail_view_widget_ = nullptr;
    lv_obj_t* confirmation_dialog_widget_ = nullptr;
    lv_obj_t* print_button_ = nullptr;

    // Pre-print option checkboxes
    lv_obj_t* bed_leveling_checkbox_ = nullptr;
    lv_obj_t* qgl_checkbox_ = nullptr;
    lv_obj_t* z_tilt_checkbox_ = nullptr;
    lv_obj_t* nozzle_clean_checkbox_ = nullptr;
    lv_obj_t* timelapse_checkbox_ = nullptr;

    // Print preparation manager (owns it)
    std::unique_ptr<PrintPreparationManager> prep_manager_;

    // === Callbacks ===
    DeleteConfirmedCallback on_delete_confirmed_;

    // === Internal Methods ===

    /**
     * @brief Map filament type string to dropdown index
     */
    [[nodiscard]] static uint32_t filament_type_to_index(const std::string& type);

    /**
     * @brief Static callback for delete confirmation
     */
    static void on_confirm_delete_static(lv_event_t* e);

    /**
     * @brief Static callback for cancel delete
     */
    static void on_cancel_delete_static(lv_event_t* e);
};

} // namespace helix::ui
