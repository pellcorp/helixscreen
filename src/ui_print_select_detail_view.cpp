// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_print_select_detail_view.h"

#include "ui_error_reporting.h"
#include "ui_modal.h"
#include "ui_nav.h"
#include "ui_print_preparation_manager.h"
#include "ui_theme.h"
#include "ui_utils.h"

#include <spdlog/spdlog.h>

namespace helix::ui {

// ============================================================================
// Lifecycle
// ============================================================================

PrintSelectDetailView::~PrintSelectDetailView() {
    // Clean up confirmation dialog if open
    if (confirmation_dialog_widget_) {
        ui_modal_hide(confirmation_dialog_widget_);
        confirmation_dialog_widget_ = nullptr;
    }

    // Clean up main widget if created
    if (detail_view_widget_) {
        lv_obj_delete(detail_view_widget_);
        detail_view_widget_ = nullptr;
    }
}

// ============================================================================
// Setup
// ============================================================================

bool PrintSelectDetailView::create(lv_obj_t* parent_screen) {
    if (!parent_screen) {
        spdlog::error("[DetailView] Cannot create: parent_screen is null");
        return false;
    }

    if (detail_view_widget_) {
        spdlog::warn("[DetailView] Detail view already exists");
        return true;
    }

    parent_screen_ = parent_screen;

    detail_view_widget_ =
        static_cast<lv_obj_t*>(lv_xml_create(parent_screen_, "print_file_detail", nullptr));

    if (!detail_view_widget_) {
        LOG_ERROR_INTERNAL("[DetailView] Failed to create detail view from XML");
        NOTIFY_ERROR("Failed to load file details");
        return false;
    }

    // Set width to fill space after nav bar
    ui_set_overlay_width(detail_view_widget_, parent_screen_);

    // Set responsive padding for content area
    lv_obj_t* content_container = lv_obj_find_by_name(detail_view_widget_, "content_container");
    if (content_container) {
        lv_coord_t padding = ui_get_header_content_padding(lv_obj_get_height(parent_screen_));
        lv_obj_set_style_pad_all(content_container, padding, 0);
    }

    lv_obj_add_flag(detail_view_widget_, LV_OBJ_FLAG_HIDDEN);

    // Store reference to print button for enable/disable state management
    print_button_ = lv_obj_find_by_name(detail_view_widget_, "print_button");

    // Look up pre-print option checkboxes
    bed_leveling_checkbox_ = lv_obj_find_by_name(detail_view_widget_, "bed_leveling_checkbox");
    qgl_checkbox_ = lv_obj_find_by_name(detail_view_widget_, "qgl_checkbox");
    z_tilt_checkbox_ = lv_obj_find_by_name(detail_view_widget_, "z_tilt_checkbox");
    nozzle_clean_checkbox_ = lv_obj_find_by_name(detail_view_widget_, "nozzle_clean_checkbox");
    timelapse_checkbox_ = lv_obj_find_by_name(detail_view_widget_, "timelapse_checkbox");

    // Initialize print preparation manager
    prep_manager_ = std::make_unique<PrintPreparationManager>();

    spdlog::debug("[DetailView] Detail view created");
    return true;
}

void PrintSelectDetailView::set_dependencies(MoonrakerAPI* api, PrinterState* printer_state) {
    api_ = api;
    printer_state_ = printer_state;

    if (prep_manager_) {
        prep_manager_->set_dependencies(api_, printer_state_);
        prep_manager_->set_checkboxes(bed_leveling_checkbox_, qgl_checkbox_, z_tilt_checkbox_,
                                      nozzle_clean_checkbox_, timelapse_checkbox_);
    }
}

// ============================================================================
// Visibility
// ============================================================================

void PrintSelectDetailView::show(const std::string& filename, const std::string& current_path,
                                 const std::string& filament_type) {
    if (!detail_view_widget_) {
        spdlog::warn("[DetailView] Cannot show: widget not created");
        return;
    }

    // Trigger async scan for embedded G-code operations (for conflict detection)
    if (!filename.empty() && prep_manager_) {
        prep_manager_->scan_file_for_operations(filename, current_path);
    }

    // Set filament type dropdown to match file metadata
    lv_obj_t* dropdown = lv_obj_find_by_name(detail_view_widget_, "filament_dropdown");
    if (dropdown && !filament_type.empty()) {
        uint32_t index = filament_type_to_index(filament_type);
        lv_dropdown_set_selected(dropdown, index);
        spdlog::debug("[DetailView] Set filament dropdown to {} (index {})", filament_type, index);
    }

    // Use nav system for consistent backdrop and z-order management
    ui_nav_push_overlay(detail_view_widget_);

    if (visible_subject_) {
        lv_subject_set_int(visible_subject_, 1);
    }

    spdlog::debug("[DetailView] Showing detail view for: {}", filename);
}

void PrintSelectDetailView::hide() {
    if (!detail_view_widget_) {
        return;
    }

    // Use nav system to properly hide and manage backdrop
    ui_nav_go_back();

    if (visible_subject_) {
        lv_subject_set_int(visible_subject_, 0);
    }

    spdlog::debug("[DetailView] Detail view hidden");
}

bool PrintSelectDetailView::is_visible() const {
    if (visible_subject_) {
        return lv_subject_get_int(visible_subject_) != 0;
    }
    return detail_view_widget_ && !lv_obj_has_flag(detail_view_widget_, LV_OBJ_FLAG_HIDDEN);
}

// ============================================================================
// Delete Confirmation
// ============================================================================

void PrintSelectDetailView::show_delete_confirmation(const std::string& filename) {
    // Configure modal
    ui_modal_config_t config = {
        .position = {.use_alignment = true, .alignment = LV_ALIGN_CENTER, .x = 0, .y = 0},
        .backdrop_opa = 180,
        .keyboard = nullptr,
        .persistent = false,
        .on_close = nullptr};

    // Create message with current filename
    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf),
             "Are you sure you want to delete '%s'? This action cannot be undone.",
             filename.c_str());

    const char* attrs[] = {"title", "Delete File?", "message", msg_buf, NULL};

    ui_modal_configure(UI_MODAL_SEVERITY_WARNING, true, "Delete", "Cancel");
    confirmation_dialog_widget_ = ui_modal_show("modal_dialog", &config, attrs);

    if (!confirmation_dialog_widget_) {
        spdlog::error("[DetailView] Failed to create confirmation dialog");
        return;
    }

    // Wire up cancel button
    lv_obj_t* cancel_btn = lv_obj_find_by_name(confirmation_dialog_widget_, "btn_secondary");
    if (cancel_btn) {
        lv_obj_add_event_cb(cancel_btn, on_cancel_delete_static, LV_EVENT_CLICKED, this);
    }

    // Wire up confirm button
    lv_obj_t* confirm_btn = lv_obj_find_by_name(confirmation_dialog_widget_, "btn_primary");
    if (confirm_btn) {
        lv_obj_add_event_cb(confirm_btn, on_confirm_delete_static, LV_EVENT_CLICKED, this);
    }

    spdlog::info("[DetailView] Delete confirmation dialog shown for: {}", filename);
}

void PrintSelectDetailView::hide_delete_confirmation() {
    if (confirmation_dialog_widget_) {
        ui_modal_hide(confirmation_dialog_widget_);
        confirmation_dialog_widget_ = nullptr;
    }
}

// ============================================================================
// Resize Handling
// ============================================================================

void PrintSelectDetailView::handle_resize(lv_obj_t* parent_screen) {
    if (!detail_view_widget_ || !parent_screen) {
        return;
    }

    lv_obj_t* content_container = lv_obj_find_by_name(detail_view_widget_, "content_container");
    if (content_container) {
        lv_coord_t padding = ui_get_header_content_padding(lv_obj_get_height(parent_screen));
        lv_obj_set_style_pad_all(content_container, padding, 0);
    }
}

// ============================================================================
// Internal Methods
// ============================================================================

uint32_t PrintSelectDetailView::filament_type_to_index(const std::string& type) {
    // Options: PLA(0), PETG(1), ABS(2), TPU(3), Nylon(4), ASA(5), PC(6)
    if (type == "PETG") {
        return 1;
    } else if (type == "ABS") {
        return 2;
    } else if (type == "TPU") {
        return 3;
    } else if (type == "Nylon" || type == "NYLON" || type == "PA") {
        return 4;
    } else if (type == "ASA") {
        return 5;
    } else if (type == "PC") {
        return 6;
    }
    // PLA is index 0 (default) for "PLA" or any unrecognized type
    return 0;
}

void PrintSelectDetailView::on_confirm_delete_static(lv_event_t* e) {
    auto* self = static_cast<PrintSelectDetailView*>(lv_event_get_user_data(e));
    if (self) {
        self->hide_delete_confirmation();
        if (self->on_delete_confirmed_) {
            self->on_delete_confirmed_();
        }
    }
}

void PrintSelectDetailView::on_cancel_delete_static(lv_event_t* e) {
    auto* self = static_cast<PrintSelectDetailView*>(lv_event_get_user_data(e));
    if (self) {
        self->hide_delete_confirmation();
    }
}

} // namespace helix::ui
