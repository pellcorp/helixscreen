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

#include "moonraker_client_mock.h"

#include <spdlog/spdlog.h>

#include <cmath>

MoonrakerClientMock::MoonrakerClientMock(PrinterType type) : printer_type_(type) {
    spdlog::info("[MoonrakerClientMock] Created with printer type: {}", static_cast<int>(type));

    // Populate hardware immediately (available for wizard without calling discover_printer())
    populate_hardware();
    spdlog::debug(
        "[MoonrakerClientMock] Hardware populated: {} heaters, {} sensors, {} fans, {} LEDs",
        heaters_.size(), sensors_.size(), fans_.size(), leds_.size());

    // Generate synthetic bed mesh data
    generate_mock_bed_mesh();
}

int MoonrakerClientMock::connect(const char* url, std::function<void()> on_connected,
                                 [[maybe_unused]] std::function<void()> on_disconnected) {
    spdlog::info("[MoonrakerClientMock] Simulating connection to: {}", url ? url : "(null)");

    // Simulate connection state change (same as real client)
    set_connection_state(ConnectionState::CONNECTING);

    // Small delay to simulate realistic connection (250ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    set_connection_state(ConnectionState::CONNECTED);

    // Immediately invoke connection callback
    if (on_connected) {
        spdlog::info("[MoonrakerClientMock] Simulated connection successful");
        on_connected();
    }

    // Store disconnect callback (never invoked in mock, but stored for consistency)
    // Note: Not needed for this simple mock implementation

    return 0; // Success
}

void MoonrakerClientMock::discover_printer(std::function<void()> on_complete) {
    spdlog::info("[MoonrakerClientMock] Simulating hardware discovery");

    // Populate hardware based on printer type
    populate_hardware();

    // Generate synthetic bed mesh data
    generate_mock_bed_mesh();

    // Log discovered hardware
    spdlog::debug("[MoonrakerClientMock] Discovered: {} heaters, {} sensors, {} fans, {} LEDs",
                  heaters_.size(), sensors_.size(), fans_.size(), leds_.size());

    // Invoke completion callback immediately (no async delay in mock)
    if (on_complete) {
        on_complete();
    }
}

void MoonrakerClientMock::populate_hardware() {
    // Clear existing data (inherited from MoonrakerClient)
    heaters_.clear();
    sensors_.clear();
    fans_.clear();
    leds_.clear();

    // Populate based on printer type
    switch (printer_type_) {
    case PrinterType::VORON_24:
        // Voron 2.4 configuration
        heaters_ = {"heater_bed", "extruder"};
        sensors_ = {"heater_bed", // Bed thermistor (Klipper naming: bare heater name)
                    "extruder",   // Hotend thermistor (Klipper naming: bare heater name)
                    "temperature_sensor chamber", "temperature_sensor raspberry_pi",
                    "temperature_sensor mcu_temp"};
        fans_ = {"heater_fan hotend_fan",
                 "fan", // Part cooling fan
                 "fan_generic nevermore", "controller_fan controller_fan"};
        leds_ = {"neopixel chamber_light", "neopixel status_led"};
        break;

    case PrinterType::VORON_TRIDENT:
        // Voron Trident configuration
        heaters_ = {"heater_bed", "extruder"};
        sensors_ = {"heater_bed", // Bed thermistor (Klipper naming: bare heater name)
                    "extruder",   // Hotend thermistor (Klipper naming: bare heater name)
                    "temperature_sensor chamber",
                    "temperature_sensor raspberry_pi",
                    "temperature_sensor mcu_temp",
                    "temperature_sensor z_thermal_adjust"};
        fans_ = {"heater_fan hotend_fan", "fan", "fan_generic exhaust_fan",
                 "controller_fan electronics_fan"};
        leds_ = {"neopixel sb_leds", "neopixel chamber_leds"};
        break;

    case PrinterType::CREALITY_K1:
        // Creality K1/K1 Max configuration
        heaters_ = {"heater_bed", "extruder"};
        sensors_ = {"heater_bed", // Bed thermistor (Klipper naming: bare heater name)
                    "extruder",   // Hotend thermistor (Klipper naming: bare heater name)
                    "temperature_sensor mcu_temp", "temperature_sensor host_temp"};
        fans_ = {"heater_fan hotend_fan", "fan", "fan_generic auxiliary_fan"};
        leds_ = {"neopixel logo_led"};
        break;

    case PrinterType::FLASHFORGE_AD5M:
        // FlashForge Adventurer 5M configuration
        heaters_ = {"heater_bed", "extruder"};
        sensors_ = {"heater_bed", // Bed thermistor (Klipper naming: bare heater name)
                    "extruder",   // Hotend thermistor (Klipper naming: bare heater name)
                    "temperature_sensor chamber", "temperature_sensor mcu_temp"};
        fans_ = {"heater_fan hotend_fan", "fan", "fan_generic chamber_fan"};
        leds_ = {"led chamber_light"};
        break;

    case PrinterType::GENERIC_COREXY:
        // Generic CoreXY printer
        heaters_ = {"heater_bed", "extruder"};
        sensors_ = {"heater_bed", // Bed thermistor (Klipper naming: bare heater name)
                    "extruder",   // Hotend thermistor (Klipper naming: bare heater name)
                    "temperature_sensor raspberry_pi"};
        fans_ = {"heater_fan hotend_fan", "fan"};
        leds_ = {};
        break;

    case PrinterType::GENERIC_BEDSLINGER:
        // Generic i3-style bedslinger
        heaters_ = {"heater_bed", "extruder"};
        sensors_ = {
            "heater_bed", // Bed thermistor (Klipper naming: bare heater name)
            "extruder"    // Hotend thermistor (Klipper naming: bare heater name)
        };
        fans_ = {"heater_fan hotend_fan", "fan"};
        leds_ = {};
        break;

    case PrinterType::MULTI_EXTRUDER:
        // Multi-extruder test case
        heaters_ = {"heater_bed", "extruder", "extruder1"};
        sensors_ = {"heater_bed", // Bed thermistor (Klipper naming: bare heater name)
                    "extruder",   // Hotend thermistor primary (Klipper naming: bare heater name)
                    "extruder1",  // Hotend thermistor secondary (Klipper naming: bare heater name)
                    "temperature_sensor chamber", "temperature_sensor mcu_temp"};
        fans_ = {"heater_fan hotend_fan", "heater_fan hotend_fan1", "fan",
                 "fan_generic exhaust_fan"};
        leds_ = {"neopixel chamber_light"};
        break;
    }

    spdlog::debug("[MoonrakerClientMock] Populated hardware:");
    for (const auto& h : heaters_)
        spdlog::debug("  Heater: {}", h);
    for (const auto& s : sensors_)
        spdlog::debug("  Sensor: {}", s);
    for (const auto& f : fans_)
        spdlog::debug("  Fan: {}", f);
    for (const auto& l : leds_)
        spdlog::debug("  LED: {}", l);
}

void MoonrakerClientMock::generate_mock_bed_mesh() {
    // Configure mesh profile
    active_bed_mesh_.name = "default";
    active_bed_mesh_.mesh_min[0] = 0.0f;
    active_bed_mesh_.mesh_min[1] = 0.0f;
    active_bed_mesh_.mesh_max[0] = 200.0f;
    active_bed_mesh_.mesh_max[1] = 200.0f;
    active_bed_mesh_.x_count = 7;
    active_bed_mesh_.y_count = 7;
    active_bed_mesh_.algo = "lagrange";

    // Generate dome-shaped mesh (matches Phase 3 test mesh for consistency)
    active_bed_mesh_.probed_matrix.clear();
    float center_x = active_bed_mesh_.x_count / 2.0f;
    float center_y = active_bed_mesh_.y_count / 2.0f;
    float max_radius = std::min(center_x, center_y);

    for (int row = 0; row < active_bed_mesh_.y_count; row++) {
        std::vector<float> row_vec;
        for (int col = 0; col < active_bed_mesh_.x_count; col++) {
            // Distance from center
            float dx = col - center_x;
            float dy = row - center_y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // Dome shape: height decreases with distance from center
            // Z values from 0.0 to 0.3mm (realistic bed mesh range)
            float normalized_dist = dist / max_radius;
            float height = 0.3f * (1.0f - normalized_dist * normalized_dist);

            row_vec.push_back(height);
        }
        active_bed_mesh_.probed_matrix.push_back(row_vec);
    }

    // Add profile names
    bed_mesh_profiles_ = {"default", "adaptive"};

    spdlog::info("[MoonrakerClientMock] Generated synthetic bed mesh: profile='{}', size={}x{}, "
                 "profiles={}",
                 active_bed_mesh_.name, active_bed_mesh_.x_count, active_bed_mesh_.y_count,
                 bed_mesh_profiles_.size());
}

void MoonrakerClientMock::disconnect() {
    spdlog::info("[MoonrakerClientMock] Simulating disconnection (no-op)");
    // Mock does nothing for disconnect - no real connection to tear down
}

int MoonrakerClientMock::send_jsonrpc(const std::string& method) {
    spdlog::debug("[MoonrakerClientMock] Mock send_jsonrpc: {}", method);
    return 0; // Success
}

int MoonrakerClientMock::send_jsonrpc(const std::string& method,
                                      [[maybe_unused]] const json& params) {
    spdlog::debug("[MoonrakerClientMock] Mock send_jsonrpc: {} (with params)", method);
    return 0; // Success
}

int MoonrakerClientMock::send_jsonrpc(const std::string& method,
                                      [[maybe_unused]] const json& params,
                                      [[maybe_unused]] std::function<void(json)> cb) {
    spdlog::debug("[MoonrakerClientMock] Mock send_jsonrpc: {} (with callback)", method);
    // Note: callback is not invoked in mock
    return 0; // Success
}

int MoonrakerClientMock::send_jsonrpc(
    const std::string& method, [[maybe_unused]] const json& params,
    [[maybe_unused]] std::function<void(json)> success_cb,
    [[maybe_unused]] std::function<void(const MoonrakerError&)> error_cb,
    [[maybe_unused]] uint32_t timeout_ms) {
    spdlog::debug("[MoonrakerClientMock] Mock send_jsonrpc: {} (with success/error callbacks)",
                  method);
    // Note: callbacks are not invoked in mock
    return 0; // Success
}

int MoonrakerClientMock::gcode_script(const std::string& gcode) {
    spdlog::debug("[MoonrakerClientMock] Mock gcode_script: {}", gcode);
    return 0; // Success
}
