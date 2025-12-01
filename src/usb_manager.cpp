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

#include "usb_manager.h"

#include <spdlog/spdlog.h>

UsbManager::UsbManager(bool force_mock) : force_mock_(force_mock) {
    spdlog::debug("[UsbManager] Created (force_mock={})", force_mock);
}

UsbManager::~UsbManager() {
    stop();
}

bool UsbManager::start() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (backend_ && backend_->is_running()) {
        spdlog::debug("[UsbManager] Already running");
        return true;
    }

    // Create backend
    backend_ = UsbBackend::create(force_mock_);
    if (!backend_) {
        spdlog::error("[UsbManager] Failed to create USB backend");
        return false;
    }

    // Set up event callback
    backend_->set_event_callback(
        [this](UsbEvent event, const UsbDrive& drive) { on_backend_event(event, drive); });

    // Start backend
    UsbError result = backend_->start();
    if (!result.success()) {
        spdlog::error("[UsbManager] Failed to start backend: {}", result.technical_msg);
        backend_.reset();
        return false;
    }

    spdlog::info("[UsbManager] Started successfully");
    return true;
}

void UsbManager::stop() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!backend_) {
        return;
    }

    backend_->stop();
    backend_.reset();
    // Guard against logging during static destruction (spdlog may be gone)
    if (spdlog::default_logger()) {
        spdlog::info("[UsbManager] Stopped");
    }
}

bool UsbManager::is_running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backend_ && backend_->is_running();
}

void UsbManager::set_drive_callback(DriveCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    drive_callback_ = std::move(callback);
}

std::vector<UsbDrive> UsbManager::get_drives() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<UsbDrive> drives;
    if (!backend_ || !backend_->is_running()) {
        return drives;
    }

    UsbError result = backend_->get_connected_drives(drives);
    if (!result.success()) {
        spdlog::warn("[UsbManager] Failed to get drives: {}", result.technical_msg);
        drives.clear();
    }

    return drives;
}

std::vector<UsbGcodeFile> UsbManager::scan_for_gcode(const std::string& mount_path,
                                                     int max_depth) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<UsbGcodeFile> files;
    if (!backend_ || !backend_->is_running()) {
        return files;
    }

    UsbError result = backend_->scan_for_gcode(mount_path, files, max_depth);
    if (!result.success()) {
        spdlog::warn("[UsbManager] Failed to scan for G-code: {}", result.technical_msg);
        files.clear();
    }

    return files;
}

UsbBackend* UsbManager::get_backend() {
    std::lock_guard<std::mutex> lock(mutex_);
    return backend_.get();
}

void UsbManager::on_backend_event(UsbEvent event, const UsbDrive& drive) {
    DriveCallback callback_copy;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_copy = drive_callback_;
    }

    // Log the event
    const char* event_name = (event == UsbEvent::DRIVE_INSERTED) ? "INSERTED" : "REMOVED";
    spdlog::info("[UsbManager] Drive {}: {} ({})", event_name, drive.label, drive.mount_path);

    // Fire callback outside lock
    if (callback_copy) {
        callback_copy(event, drive);
    }
}
