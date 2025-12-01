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

#include "usb_backend_mock.h"

#include <spdlog/spdlog.h>

#include <algorithm>

UsbBackendMock::UsbBackendMock() : running_(false) {
    spdlog::debug("[UsbBackendMock] Created");
}

UsbBackendMock::~UsbBackendMock() {
    stop();
}

UsbError UsbBackendMock::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return UsbError(UsbResult::SUCCESS);
    }

    running_ = true;
    spdlog::info("[UsbBackendMock] Started - mock USB monitoring active");
    return UsbError(UsbResult::SUCCESS);
}

void UsbBackendMock::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_) {
        return;
    }

    running_ = false;
    // Guard against logging during static destruction (spdlog may be gone)
    if (spdlog::default_logger()) {
        spdlog::info("[UsbBackendMock] Stopped");
    }
}

bool UsbBackendMock::is_running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void UsbBackendMock::set_event_callback(EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    event_callback_ = std::move(callback);
}

UsbError UsbBackendMock::get_connected_drives(std::vector<UsbDrive>& drives) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!running_) {
        return UsbError(UsbResult::NOT_INITIALIZED, "Backend not started",
                        "USB monitoring not active");
    }

    drives = drives_;
    return UsbError(UsbResult::SUCCESS);
}

UsbError UsbBackendMock::scan_for_gcode(const std::string& mount_path,
                                        std::vector<UsbGcodeFile>& files, int max_depth) {
    std::lock_guard<std::mutex> lock(mutex_);
    (void)max_depth; // Unused in mock

    if (!running_) {
        return UsbError(UsbResult::NOT_INITIALIZED, "Backend not started",
                        "USB monitoring not active");
    }

    // Check if drive exists
    auto drive_it = std::find_if(drives_.begin(), drives_.end(), [&mount_path](const UsbDrive& d) {
        return d.mount_path == mount_path;
    });

    if (drive_it == drives_.end()) {
        return UsbError(UsbResult::DRIVE_NOT_FOUND, "Drive not found: " + mount_path,
                        "USB drive not connected");
    }

    // Return mock files for this drive
    auto files_it = mock_files_.find(mount_path);
    if (files_it != mock_files_.end()) {
        files = files_it->second;
    } else {
        files.clear();
    }

    spdlog::debug("[UsbBackendMock] Scan returned {} files for {}", files.size(), mount_path);
    return UsbError(UsbResult::SUCCESS);
}

void UsbBackendMock::simulate_drive_insert(const UsbDrive& drive) {
    EventCallback callback_copy;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Check if drive already exists
        auto it = std::find_if(drives_.begin(), drives_.end(), [&drive](const UsbDrive& d) {
            return d.mount_path == drive.mount_path;
        });

        if (it != drives_.end()) {
            spdlog::warn("[UsbBackendMock] Drive already inserted: {}", drive.mount_path);
            return;
        }

        drives_.push_back(drive);
        callback_copy = event_callback_;
        spdlog::info("[UsbBackendMock] Drive inserted: {} ({})", drive.label, drive.mount_path);
    }

    // Fire callback outside lock to avoid deadlock
    if (callback_copy) {
        callback_copy(UsbEvent::DRIVE_INSERTED, drive);
    }
}

void UsbBackendMock::simulate_drive_remove(const std::string& mount_path) {
    EventCallback callback_copy;
    UsbDrive removed_drive;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = std::find_if(drives_.begin(), drives_.end(), [&mount_path](const UsbDrive& d) {
            return d.mount_path == mount_path;
        });

        if (it == drives_.end()) {
            spdlog::warn("[UsbBackendMock] Drive not found for removal: {}", mount_path);
            return;
        }

        removed_drive = *it;
        drives_.erase(it);
        mock_files_.erase(mount_path);
        callback_copy = event_callback_;
        spdlog::info("[UsbBackendMock] Drive removed: {} ({})", removed_drive.label, mount_path);
    }

    // Fire callback outside lock to avoid deadlock
    if (callback_copy) {
        callback_copy(UsbEvent::DRIVE_REMOVED, removed_drive);
    }
}

void UsbBackendMock::set_mock_files(const std::string& mount_path,
                                    const std::vector<UsbGcodeFile>& files) {
    std::lock_guard<std::mutex> lock(mutex_);
    mock_files_[mount_path] = files;
    spdlog::debug("[UsbBackendMock] Set {} mock files for {}", files.size(), mount_path);
}

void UsbBackendMock::clear_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    drives_.clear();
    mock_files_.clear();
    spdlog::debug("[UsbBackendMock] Cleared all drives and files");
}

void UsbBackendMock::add_demo_drives() {
    // Add a demo USB drive with realistic G-code files
    UsbDrive demo_drive("/media/usb0", "/dev/sda1", "PRINT_FILES",
                        16ULL * 1024 * 1024 * 1024, // 16 GB total
                        8ULL * 1024 * 1024 * 1024); // 8 GB available

    simulate_drive_insert(demo_drive);

    // Add demo G-code files
    int64_t now = std::time(nullptr);
    std::vector<UsbGcodeFile> demo_files = {
        {"/media/usb0/benchy.gcode", "benchy.gcode", 2ULL * 1024 * 1024, now - 86400},
        {"/media/usb0/calibration_cube.gcode", "calibration_cube.gcode", 512 * 1024, now - 172800},
        {"/media/usb0/phone_stand_v2.gcode", "phone_stand_v2.gcode", 5ULL * 1024 * 1024,
         now - 259200},
        {"/media/usb0/cable_clip_x10.gcode", "cable_clip_x10.gcode", 1ULL * 1024 * 1024,
         now - 345600},
        {"/media/usb0/projects/enclosure_top.gcode", "enclosure_top.gcode", 15ULL * 1024 * 1024,
         now - 432000},
        {"/media/usb0/projects/enclosure_bottom.gcode", "enclosure_bottom.gcode",
         12ULL * 1024 * 1024, now - 518400},
    };

    set_mock_files(demo_drive.mount_path, demo_files);

    spdlog::info("[UsbBackendMock] Added demo drive with {} files", demo_files.size());
}
