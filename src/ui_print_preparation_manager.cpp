// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_print_preparation_manager.h"

#include "ui_error_reporting.h"
#include "ui_panel_print_status.h"

#include <spdlog/spdlog.h>

// Forward declaration for global print status panel (declared in ui_panel_print_status.h)
PrintStatusPanel& get_global_print_status_panel();

namespace helix::ui {

// ============================================================================
// Construction / Destruction
// ============================================================================

PrintPreparationManager::~PrintPreparationManager() {
    // Cancel any running sequence
    if (pre_print_sequencer_) {
        pre_print_sequencer_.reset();
    }
}

// ============================================================================
// Setup
// ============================================================================

void PrintPreparationManager::set_dependencies(MoonrakerAPI* api, PrinterState* printer_state) {
    api_ = api;
    printer_state_ = printer_state;
}

void PrintPreparationManager::set_checkboxes(lv_obj_t* bed_leveling, lv_obj_t* qgl,
                                              lv_obj_t* z_tilt, lv_obj_t* nozzle_clean,
                                              lv_obj_t* timelapse) {
    bed_leveling_checkbox_ = bed_leveling;
    qgl_checkbox_ = qgl;
    z_tilt_checkbox_ = z_tilt;
    nozzle_clean_checkbox_ = nozzle_clean;
    timelapse_checkbox_ = timelapse;
}

// ============================================================================
// G-code Scanning
// ============================================================================

void PrintPreparationManager::scan_file_for_operations(const std::string& filename,
                                                        const std::string& current_path) {
    // Skip if already cached for this file
    if (cached_scan_filename_ == filename && cached_scan_result_.has_value()) {
        spdlog::debug("[PrintPreparationManager] Using cached scan result for {}", filename);
        return;
    }

    if (!api_) {
        spdlog::warn("[PrintPreparationManager] Cannot scan G-code - no API connection");
        return;
    }

    // Build path for download
    std::string file_path = current_path.empty() ? filename : current_path + "/" + filename;

    spdlog::info("[PrintPreparationManager] Scanning G-code for embedded operations: {}",
                 file_path);

    auto* self = this;
    api_->download_file(
        "gcodes", file_path,
        // Success: parse content and cache result
        [self, filename](const std::string& content) {
            gcode::GCodeOpsDetector detector;
            self->cached_scan_result_ = detector.scan_content(content);
            self->cached_scan_filename_ = filename;

            if (self->cached_scan_result_->operations.empty()) {
                spdlog::debug("[PrintPreparationManager] No embedded operations found in {}",
                              filename);
            } else {
                spdlog::info("[PrintPreparationManager] Found {} embedded operations in {}:",
                             self->cached_scan_result_->operations.size(), filename);
                for (const auto& op : self->cached_scan_result_->operations) {
                    spdlog::info("[PrintPreparationManager]   - {} at line {} ({})",
                                 op.display_name(), op.line_number, op.raw_line.substr(0, 50));
                }
            }
        },
        // Error: just log, don't block the UI
        [self, filename](const MoonrakerError& error) {
            spdlog::warn("[PrintPreparationManager] Failed to scan G-code {}: {}", filename,
                         error.message);
            self->cached_scan_result_.reset();
            self->cached_scan_filename_.clear();
        });
}

void PrintPreparationManager::clear_scan_cache() {
    cached_scan_result_.reset();
    cached_scan_filename_.clear();
}

bool PrintPreparationManager::has_scan_result_for(const std::string& filename) const {
    return cached_scan_filename_ == filename && cached_scan_result_.has_value();
}

// ============================================================================
// Print Execution
// ============================================================================

PrePrintOptions PrintPreparationManager::read_options_from_checkboxes() const {
    PrePrintOptions options;

    auto is_checked = [](lv_obj_t* checkbox) -> bool {
        return checkbox && lv_obj_has_state(checkbox, LV_STATE_CHECKED);
    };

    options.bed_leveling = is_checked(bed_leveling_checkbox_);
    options.qgl = is_checked(qgl_checkbox_);
    options.z_tilt = is_checked(z_tilt_checkbox_);
    options.nozzle_clean = is_checked(nozzle_clean_checkbox_);
    options.timelapse = is_checked(timelapse_checkbox_);

    return options;
}

void PrintPreparationManager::start_print(const std::string& filename,
                                           const std::string& current_path,
                                           NavigateToStatusCallback on_navigate_to_status,
                                           PreparingCallback on_preparing,
                                           PreparingProgressCallback on_progress,
                                           PrintCompletionCallback on_completion) {
    if (!api_) {
        spdlog::error("[PrintPreparationManager] Cannot start print - not connected to printer");
        NOTIFY_ERROR("Cannot start print: not connected to printer");
        if (on_completion) {
            on_completion(false, "Not connected to printer");
        }
        return;
    }

    // Build full path for print
    std::string filename_to_print =
        current_path.empty() ? filename : current_path + "/" + filename;

    // Read checkbox states
    PrePrintOptions options = read_options_from_checkboxes();
    bool has_pre_print_ops =
        options.bed_leveling || options.qgl || options.z_tilt || options.nozzle_clean;

    spdlog::info("[PrintPreparationManager] Starting print: {} (pre-print: mesh={}, qgl={}, "
                 "z_tilt={}, clean={}, timelapse={})",
                 filename_to_print, options.bed_leveling, options.qgl, options.z_tilt,
                 options.nozzle_clean, options.timelapse);

    // Enable timelapse recording if requested (Moonraker-Timelapse plugin)
    if (options.timelapse) {
        api_->set_timelapse_enabled(
            true,
            []() { spdlog::info("[PrintPreparationManager] Timelapse enabled for this print"); },
            [](const MoonrakerError& err) {
                spdlog::error("[PrintPreparationManager] Failed to enable timelapse: {}",
                              err.message);
            });
    }

    // Check if user disabled operations that are embedded in the G-code file
    std::vector<gcode::OperationType> ops_to_disable = collect_ops_to_disable();

    if (!ops_to_disable.empty()) {
        spdlog::info("[PrintPreparationManager] User disabled {} embedded operations - modifying "
                     "G-code",
                     ops_to_disable.size());
        modify_and_print(filename_to_print, ops_to_disable, on_navigate_to_status);
        return; // modify_and_print handles everything including navigation
    }

    if (has_pre_print_ops) {
        execute_pre_print_sequence(filename_to_print, options, on_navigate_to_status, on_preparing,
                                   on_progress, on_completion);
    } else {
        start_print_directly(filename_to_print, on_navigate_to_status, on_completion);
    }
}

void PrintPreparationManager::cancel_preparation() {
    if (pre_print_sequencer_) {
        spdlog::info("[PrintPreparationManager] Cancelling pre-print sequence");
        pre_print_sequencer_.reset();
    }
}

// ============================================================================
// Internal Methods
// ============================================================================

bool PrintPreparationManager::is_option_disabled(lv_obj_t* checkbox) {
    if (!checkbox)
        return false;
    bool is_visible = !lv_obj_has_flag(checkbox, LV_OBJ_FLAG_HIDDEN);
    bool is_checked = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
    return is_visible && !is_checked; // Visible but NOT checked = disabled
}

std::vector<gcode::OperationType> PrintPreparationManager::collect_ops_to_disable() const {
    std::vector<gcode::OperationType> ops_to_disable;

    if (!cached_scan_result_.has_value()) {
        return ops_to_disable; // No scan result, nothing to disable
    }

    // Check each operation type: if file has it embedded AND user disabled it
    if (is_option_disabled(bed_leveling_checkbox_) &&
        cached_scan_result_->has_operation(gcode::OperationType::BED_LEVELING)) {
        ops_to_disable.push_back(gcode::OperationType::BED_LEVELING);
        spdlog::debug("[PrintPreparationManager] User disabled bed leveling, file has it embedded");
    }

    if (is_option_disabled(qgl_checkbox_) &&
        cached_scan_result_->has_operation(gcode::OperationType::QGL)) {
        ops_to_disable.push_back(gcode::OperationType::QGL);
        spdlog::debug("[PrintPreparationManager] User disabled QGL, file has it embedded");
    }

    if (is_option_disabled(z_tilt_checkbox_) &&
        cached_scan_result_->has_operation(gcode::OperationType::Z_TILT)) {
        ops_to_disable.push_back(gcode::OperationType::Z_TILT);
        spdlog::debug("[PrintPreparationManager] User disabled Z-tilt, file has it embedded");
    }

    if (is_option_disabled(nozzle_clean_checkbox_) &&
        cached_scan_result_->has_operation(gcode::OperationType::NOZZLE_CLEAN)) {
        ops_to_disable.push_back(gcode::OperationType::NOZZLE_CLEAN);
        spdlog::debug("[PrintPreparationManager] User disabled nozzle clean, file has it embedded");
    }

    return ops_to_disable;
}

void PrintPreparationManager::modify_and_print(
    const std::string& file_path, const std::vector<gcode::OperationType>& ops_to_disable,
    NavigateToStatusCallback on_navigate_to_status) {
    if (!api_) {
        NOTIFY_ERROR("Cannot start print - not connected to printer");
        return;
    }

    if (!cached_scan_result_.has_value()) {
        spdlog::error("[PrintPreparationManager] modify_and_print called without scan result");
        NOTIFY_ERROR("Internal error: no scan result");
        return;
    }

    spdlog::info("[PrintPreparationManager] Modifying G-code to disable {} operations",
                 ops_to_disable.size());

    auto* self = this;
    auto scan_result = cached_scan_result_; // Copy for lambda capture

    // Step 1: Download the original file
    api_->download_file(
        "gcodes", file_path,
        // Success: modify and either use plugin or legacy flow
        [self, file_path, ops_to_disable, on_navigate_to_status,
         scan_result](const std::string& content) {
            // Step 2: Apply modifications
            gcode::GCodeFileModifier modifier;
            modifier.disable_operations(*scan_result, ops_to_disable);

            std::string modified_content = modifier.apply_to_content(content);
            if (modified_content.empty()) {
                NOTIFY_ERROR("Failed to modify G-code file");
                return;
            }

            // Build modification identifiers for plugin
            std::vector<std::string> mod_names;
            for (const auto& op : ops_to_disable) {
                mod_names.push_back(gcode::GCodeOpsDetector::operation_type_name(op) + "_disabled");
            }

            // Extract just the filename for display purposes
            size_t last_slash = file_path.rfind('/');
            std::string display_filename =
                (last_slash != std::string::npos) ? file_path.substr(last_slash + 1) : file_path;

            // Check if helix_print plugin is available
            if (self->api_->has_helix_plugin()) {
                // NEW PATH: Use helix_print plugin (single API call)
                spdlog::info("[PrintPreparationManager] Using helix_print plugin for modified "
                             "print");

                self->api_->start_modified_print(
                    file_path, modified_content, mod_names,
                    // Success callback
                    [on_navigate_to_status, display_filename](const ModifiedPrintResult& result) {
                        spdlog::info("[PrintPreparationManager] Print started via helix_print "
                                     "plugin: {} -> {}",
                                     result.original_filename, result.print_filename);

                        // Set thumbnail source to original filename
                        get_global_print_status_panel().set_thumbnail_source(display_filename);

                        if (on_navigate_to_status) {
                            on_navigate_to_status();
                        }
                    },
                    // Error callback
                    [file_path](const MoonrakerError& error) {
                        NOTIFY_ERROR("Failed to start modified print: {}", error.message);
                        LOG_ERROR_INTERNAL(
                            "[PrintPreparationManager] helix_print plugin error for {}: {}",
                            file_path, error.message);
                    });
            } else {
                // LEGACY PATH: Upload to .helix_temp then start print
                spdlog::info("[PrintPreparationManager] Using legacy flow (helix_print plugin not "
                             "available)");

                // Generate unique temp filename
                std::string temp_filename = ".helix_temp/modified_" +
                                            std::to_string(std::time(nullptr)) + "_" +
                                            display_filename;

                spdlog::info("[PrintPreparationManager] Uploading modified G-code to {}",
                             temp_filename);

                self->api_->upload_file_with_name(
                    "gcodes", temp_filename, temp_filename, modified_content,
                    // Success: start print with modified file
                    [self, temp_filename, display_filename, on_navigate_to_status]() {
                        spdlog::info("[PrintPreparationManager] Modified file uploaded, starting "
                                     "print");

                        // Set thumbnail source to original filename before starting print
                        get_global_print_status_panel().set_thumbnail_source(display_filename);

                        // Start print with the modified file
                        self->api_->start_print(
                            temp_filename,
                            [on_navigate_to_status, display_filename]() {
                                spdlog::info("[PrintPreparationManager] Print started with "
                                             "modified G-code (original: {})",
                                             display_filename);
                                if (on_navigate_to_status) {
                                    on_navigate_to_status();
                                }
                            },
                            [temp_filename](const MoonrakerError& error) {
                                NOTIFY_ERROR("Failed to start print: {}", error.message);
                                LOG_ERROR_INTERNAL(
                                    "[PrintPreparationManager] Print start failed for {}: {}",
                                    temp_filename, error.message);
                            });
                    },
                    // Error uploading
                    [](const MoonrakerError& error) {
                        NOTIFY_ERROR("Failed to upload modified G-code: {}", error.message);
                        LOG_ERROR_INTERNAL("[PrintPreparationManager] Upload failed: {}",
                                           error.message);
                    });
            }
        },
        // Error downloading
        [file_path](const MoonrakerError& error) {
            NOTIFY_ERROR("Failed to download G-code for modification: {}", error.message);
            LOG_ERROR_INTERNAL("[PrintPreparationManager] Download failed for {}: {}", file_path,
                               error.message);
        });
}

void PrintPreparationManager::execute_pre_print_sequence(
    const std::string& filename, const PrePrintOptions& options,
    NavigateToStatusCallback on_navigate_to_status, PreparingCallback on_preparing,
    PreparingProgressCallback on_progress, PrintCompletionCallback on_completion) {

    // Create command sequencer for pre-print operations
    pre_print_sequencer_ =
        std::make_unique<gcode::CommandSequencer>(api_->get_client(), *api_, *printer_state_);

    // Always home first if doing any pre-print operations
    pre_print_sequencer_->add_operation(gcode::OperationType::HOMING, {}, "Homing");

    // Add selected operations in logical order
    if (options.qgl) {
        pre_print_sequencer_->add_operation(gcode::OperationType::QGL, {}, "Quad Gantry Level");
    }
    if (options.z_tilt) {
        pre_print_sequencer_->add_operation(gcode::OperationType::Z_TILT, {}, "Z-Tilt Adjust");
    }
    if (options.bed_leveling) {
        pre_print_sequencer_->add_operation(gcode::OperationType::BED_LEVELING, {},
                                            "Bed Mesh Calibration");
    }
    if (options.nozzle_clean) {
        pre_print_sequencer_->add_operation(gcode::OperationType::NOZZLE_CLEAN, {}, "Clean Nozzle");
    }

    // Add the actual print start as the final operation
    gcode::OperationParams print_params;
    print_params.filename = filename;
    pre_print_sequencer_->add_operation(gcode::OperationType::START_PRINT, print_params,
                                        "Starting Print");

    int queue_size = static_cast<int>(pre_print_sequencer_->queue_size());

    // Navigate to print status panel in "Preparing" state
    if (on_navigate_to_status) {
        on_navigate_to_status();
    }

    // Initialize the preparing state
    auto& status_panel = get_global_print_status_panel();
    status_panel.set_preparing("Starting...", 0, queue_size);

    auto* self = this;

    // Start the sequence
    pre_print_sequencer_->start(
        // Progress callback - update the Preparing UI
        [on_preparing, on_progress](const std::string& op_name, int step, int total,
                                    float progress) {
            spdlog::info("[PrintPreparationManager] Pre-print progress: {} ({}/{}, {:.0f}%)",
                         op_name, step, total, progress * 100.0f);

            // Update PrintStatusPanel's preparing state
            auto& status_panel = get_global_print_status_panel();
            status_panel.set_preparing(op_name, step, total);
            status_panel.set_preparing_progress(progress);

            if (on_preparing) {
                on_preparing(op_name, step, total);
            }
            if (on_progress) {
                on_progress(progress);
            }
        },
        // Completion callback
        [self, on_completion](bool success, const std::string& error) {
            auto& status_panel = get_global_print_status_panel();

            if (success) {
                spdlog::info("[PrintPreparationManager] Pre-print sequence complete, print started");
                // Transition from Preparing → Printing state
                status_panel.end_preparing(true);
            } else {
                NOTIFY_ERROR("Pre-print failed: {}", error);
                LOG_ERROR_INTERNAL("[PrintPreparationManager] Pre-print sequence failed: {}",
                                   error);
                // Transition from Preparing → Idle state
                status_panel.end_preparing(false);
            }

            // Clean up sequencer
            self->pre_print_sequencer_.reset();

            if (on_completion) {
                on_completion(success, error);
            }
        });
}

void PrintPreparationManager::start_print_directly(const std::string& filename,
                                                    NavigateToStatusCallback on_navigate_to_status,
                                                    PrintCompletionCallback on_completion) {
    api_->start_print(
        filename,
        // Success callback
        [on_navigate_to_status, on_completion]() {
            spdlog::info("[PrintPreparationManager] Print started successfully");

            if (on_navigate_to_status) {
                on_navigate_to_status();
            }

            if (on_completion) {
                on_completion(true, "");
            }
        },
        // Error callback
        [filename, on_completion](const MoonrakerError& error) {
            NOTIFY_ERROR("Failed to start print: {}", error.message);
            LOG_ERROR_INTERNAL("[PrintPreparationManager] Print start failed for {}: {} ({})",
                               filename, error.message, error.get_type_string());

            if (on_completion) {
                on_completion(false, error.message);
            }
        });
}

} // namespace helix::ui
