// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Copyright (C) 2025 356C LLC
 * Author: Preston Brown <pbrown@brown-house.net>
 *
 * This file is part of HelixScreen.
 *
 * HelixScreen is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HelixScreen is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HelixScreen. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ams_state.h"

#include <spdlog/spdlog.h>

#include <cstring>

AmsState& AmsState::instance() {
    static AmsState instance;
    return instance;
}

AmsState::AmsState() {
    std::memset(action_detail_buf_, 0, sizeof(action_detail_buf_));
}

AmsState::~AmsState() {
    if (backend_) {
        backend_->stop();
    }
}

void AmsState::init_subjects(bool register_xml) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return;
    }

    spdlog::debug("AmsState: Initializing subjects");

    // System-level subjects
    lv_subject_init_int(&ams_type_, static_cast<int>(AmsType::NONE));
    lv_subject_init_int(&ams_action_, static_cast<int>(AmsAction::IDLE));
    lv_subject_init_int(&current_gate_, -1);
    lv_subject_init_int(&current_tool_, -1);
    lv_subject_init_int(&filament_loaded_, 0);
    lv_subject_init_int(&gate_count_, 0);
    lv_subject_init_int(&gates_version_, 0);

    // String subject for action detail
    lv_subject_init_string(&ams_action_detail_, action_detail_buf_, nullptr,
                           sizeof(action_detail_buf_), "");

    // Per-gate subjects
    for (int i = 0; i < MAX_GATES; ++i) {
        lv_subject_init_int(&gate_colors_[i], static_cast<int>(AMS_DEFAULT_GATE_COLOR));
        lv_subject_init_int(&gate_statuses_[i], static_cast<int>(GateStatus::UNKNOWN));
    }

    // Register with XML system if requested
    if (register_xml) {
        lv_xml_register_subject(NULL, "ams_type", &ams_type_);
        lv_xml_register_subject(NULL, "ams_action", &ams_action_);
        lv_xml_register_subject(NULL, "ams_action_detail", &ams_action_detail_);
        lv_xml_register_subject(NULL, "ams_current_gate", &current_gate_);
        lv_xml_register_subject(NULL, "ams_current_tool", &current_tool_);
        lv_xml_register_subject(NULL, "ams_filament_loaded", &filament_loaded_);
        lv_xml_register_subject(NULL, "ams_gate_count", &gate_count_);
        lv_xml_register_subject(NULL, "ams_gates_version", &gates_version_);

        // Register per-gate subjects with indexed names
        char name_buf[32];
        for (int i = 0; i < MAX_GATES; ++i) {
            snprintf(name_buf, sizeof(name_buf), "ams_gate_%d_color", i);
            lv_xml_register_subject(NULL, name_buf, &gate_colors_[i]);

            snprintf(name_buf, sizeof(name_buf), "ams_gate_%d_status", i);
            lv_xml_register_subject(NULL, name_buf, &gate_statuses_[i]);
        }

        spdlog::info("AmsState: Registered {} system subjects and {} per-gate subjects", 8,
                     MAX_GATES * 2);
    }

    initialized_ = true;
}

void AmsState::reset_for_testing() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = false;
    backend_.reset();
}

void AmsState::set_backend(std::unique_ptr<AmsBackend> backend) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Stop existing backend
    if (backend_) {
        backend_->stop();
    }

    backend_ = std::move(backend);

    if (backend_) {
        // Register event callback
        backend_->set_event_callback([this](const std::string& event, const std::string& data) {
            on_backend_event(event, data);
        });

        spdlog::info("AmsState: Backend set (type={})", ams_type_to_string(backend_->get_type()));
    }
}

AmsBackend* AmsState::get_backend() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backend_.get();
}

bool AmsState::is_available() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backend_ && backend_->get_type() != AmsType::NONE;
}

lv_subject_t* AmsState::get_gate_color_subject(int gate_index) {
    if (gate_index < 0 || gate_index >= MAX_GATES) {
        return nullptr;
    }
    return &gate_colors_[gate_index];
}

lv_subject_t* AmsState::get_gate_status_subject(int gate_index) {
    if (gate_index < 0 || gate_index >= MAX_GATES) {
        return nullptr;
    }
    return &gate_statuses_[gate_index];
}

void AmsState::sync_from_backend() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!backend_) {
        return;
    }

    AmsSystemInfo info = backend_->get_system_info();

    // Update system-level subjects
    lv_subject_set_int(&ams_type_, static_cast<int>(info.type));
    lv_subject_set_int(&ams_action_, static_cast<int>(info.action));
    lv_subject_set_int(&current_gate_, info.current_gate);
    lv_subject_set_int(&current_tool_, info.current_tool);
    lv_subject_set_int(&filament_loaded_, info.filament_loaded ? 1 : 0);
    lv_subject_set_int(&gate_count_, info.total_gates);

    // Update action detail string
    if (!info.operation_detail.empty()) {
        lv_subject_copy_string(&ams_action_detail_, info.operation_detail.c_str());
    } else {
        lv_subject_copy_string(&ams_action_detail_, ams_action_to_string(info.action));
    }

    // Update per-gate subjects
    for (int i = 0; i < std::min(info.total_gates, MAX_GATES); ++i) {
        const GateInfo* gate = info.get_gate_global(i);
        if (gate) {
            lv_subject_set_int(&gate_colors_[i], static_cast<int>(gate->color_rgb));
            lv_subject_set_int(&gate_statuses_[i], static_cast<int>(gate->status));
        }
    }

    // Clear remaining gate subjects
    for (int i = info.total_gates; i < MAX_GATES; ++i) {
        lv_subject_set_int(&gate_colors_[i], static_cast<int>(AMS_DEFAULT_GATE_COLOR));
        lv_subject_set_int(&gate_statuses_[i], static_cast<int>(GateStatus::UNKNOWN));
    }

    bump_gates_version();

    spdlog::debug("AmsState: Synced from backend - type={}, gates={}, action={}",
                  ams_type_to_string(info.type), info.total_gates,
                  ams_action_to_string(info.action));
}

void AmsState::update_gate(int gate_index) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!backend_ || gate_index < 0 || gate_index >= MAX_GATES) {
        return;
    }

    GateInfo gate = backend_->get_gate_info(gate_index);
    if (gate.gate_index >= 0) {
        lv_subject_set_int(&gate_colors_[gate_index], static_cast<int>(gate.color_rgb));
        lv_subject_set_int(&gate_statuses_[gate_index], static_cast<int>(gate.status));
        bump_gates_version();

        spdlog::debug("AmsState: Updated gate {} - color=0x{:06X}, status={}", gate_index,
                      gate.color_rgb, gate_status_to_string(gate.status));
    }
}

void AmsState::on_backend_event(const std::string& event, const std::string& data) {
    spdlog::debug("AmsState: Received event '{}' data='{}'", event, data);

    if (event == AmsBackend::EVENT_STATE_CHANGED) {
        sync_from_backend();
    } else if (event == AmsBackend::EVENT_GATE_CHANGED) {
        // Parse gate index from data
        if (!data.empty()) {
            try {
                int gate_index = std::stoi(data);
                update_gate(gate_index);
            } catch (...) {
                // Invalid data, do full sync
                sync_from_backend();
            }
        }
    } else if (event == AmsBackend::EVENT_LOAD_COMPLETE ||
               event == AmsBackend::EVENT_UNLOAD_COMPLETE ||
               event == AmsBackend::EVENT_TOOL_CHANGED) {
        // These events indicate state change, sync everything
        sync_from_backend();
    } else if (event == AmsBackend::EVENT_ERROR) {
        // Error occurred, sync to get error state
        sync_from_backend();
        spdlog::warn("AmsState: Backend error - {}", data);
    } else if (event == AmsBackend::EVENT_ATTENTION_REQUIRED) {
        // User intervention needed
        sync_from_backend();
        spdlog::warn("AmsState: Attention required - {}", data);
    }
}

void AmsState::bump_gates_version() {
    int current = lv_subject_get_int(&gates_version_);
    lv_subject_set_int(&gates_version_, current + 1);
}
