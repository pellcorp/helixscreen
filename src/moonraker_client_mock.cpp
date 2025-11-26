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

MoonrakerClientMock::~MoonrakerClientMock() {
    stop_temperature_simulation();
}

int MoonrakerClientMock::connect(const char* url, std::function<void()> on_connected,
                                 [[maybe_unused]] std::function<void()> on_disconnected) {
    spdlog::info("[MoonrakerClientMock] Simulating connection to: {}", url ? url : "(null)");

    // Simulate connection state change (same as real client)
    set_connection_state(ConnectionState::CONNECTING);

    // Small delay to simulate realistic connection (250ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    set_connection_state(ConnectionState::CONNECTED);

    // Start temperature simulation
    start_temperature_simulation();

    // Dispatch initial state BEFORE calling on_connected (matches real Moonraker behavior)
    // Real client sends initial state from subscription response - mock does it here
    dispatch_initial_state();

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
    spdlog::info("[MoonrakerClientMock] Simulating disconnection");
    stop_temperature_simulation();
    set_connection_state(ConnectionState::DISCONNECTED);
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
    // STUB: Callback NOT invoked - caller will not receive response!
    // TODO: Implement response simulation for specific methods:
    //   - server.files.list → mock file list
    //   - server.files.metadata → mock file metadata
    //   - printer.objects.query → mock printer state
    spdlog::warn("[MoonrakerClientMock] STUB: Callback for '{}' NOT INVOKED - response simulation not implemented", method);
    return 0; // Success
}

int MoonrakerClientMock::send_jsonrpc(
    const std::string& method, [[maybe_unused]] const json& params,
    [[maybe_unused]] std::function<void(json)> success_cb,
    [[maybe_unused]] std::function<void(const MoonrakerError&)> error_cb,
    [[maybe_unused]] uint32_t timeout_ms) {
    spdlog::debug("[MoonrakerClientMock] Mock send_jsonrpc: {} (with success/error callbacks)",
                  method);
    // STUB: Callbacks NOT invoked - caller will not receive response or error!
    // TODO: Implement response simulation for specific methods
    spdlog::warn("[MoonrakerClientMock] STUB: Callbacks for '{}' NOT INVOKED - response simulation not implemented", method);
    return 0; // Success
}

int MoonrakerClientMock::gcode_script(const std::string& gcode) {
    spdlog::debug("[MoonrakerClientMock] Mock gcode_script: {}", gcode);

    // Parse temperature commands to update simulation targets
    // M104 Sxxx - Set extruder temp (no wait)
    // M109 Sxxx - Set extruder temp (wait)
    // M140 Sxxx - Set bed temp (no wait)
    // M190 Sxxx - Set bed temp (wait)
    // SET_HEATER_TEMPERATURE HEATER=extruder TARGET=xxx
    // SET_HEATER_TEMPERATURE HEATER=heater_bed TARGET=xxx

    // Check for Klipper-style SET_HEATER_TEMPERATURE commands
    if (gcode.find("SET_HEATER_TEMPERATURE") != std::string::npos) {
        double target = 0.0;
        size_t target_pos = gcode.find("TARGET=");
        if (target_pos != std::string::npos) {
            target = std::stod(gcode.substr(target_pos + 7));
        }

        if (gcode.find("HEATER=extruder") != std::string::npos) {
            set_extruder_target(target);
            spdlog::info("[MoonrakerClientMock] Extruder target set to {}°C", target);
        } else if (gcode.find("HEATER=heater_bed") != std::string::npos) {
            set_bed_target(target);
            spdlog::info("[MoonrakerClientMock] Bed target set to {}°C", target);
        }
    }
    // Check for M-code style temperature commands
    else if (gcode.find("M104") != std::string::npos || gcode.find("M109") != std::string::npos) {
        size_t s_pos = gcode.find('S');
        if (s_pos != std::string::npos) {
            double target = std::stod(gcode.substr(s_pos + 1));
            set_extruder_target(target);
            spdlog::info("[MoonrakerClientMock] Extruder target set to {}°C (M-code)", target);
        }
    } else if (gcode.find("M140") != std::string::npos || gcode.find("M190") != std::string::npos) {
        size_t s_pos = gcode.find('S');
        if (s_pos != std::string::npos) {
            double target = std::stod(gcode.substr(s_pos + 1));
            set_bed_target(target);
            spdlog::info("[MoonrakerClientMock] Bed target set to {}°C (M-code)", target);
        }
    }

    // Parse motion mode commands (G90/G91)
    // G90 - Absolute positioning mode
    // G91 - Relative positioning mode
    if (gcode.find("G90") != std::string::npos) {
        relative_mode_.store(false);
        spdlog::info("[MoonrakerClientMock] Set absolute positioning mode (G90)");
    } else if (gcode.find("G91") != std::string::npos) {
        relative_mode_.store(true);
        spdlog::info("[MoonrakerClientMock] Set relative positioning mode (G91)");
    }

    // Parse homing command (G28)
    // G28 - Home all axes
    // G28 X - Home X axis only
    // G28 Y - Home Y axis only
    // G28 Z - Home Z axis only
    // G28 X Y - Home X and Y axes
    if (gcode.find("G28") != std::string::npos) {
        // Check if specific axes are mentioned after G28
        // Need to look after the G28 to avoid false matches
        size_t g28_pos = gcode.find("G28");
        std::string after_g28 = gcode.substr(g28_pos + 3);

        // Check for specific axis letters (case insensitive search)
        bool has_x = after_g28.find('X') != std::string::npos ||
                     after_g28.find('x') != std::string::npos;
        bool has_y = after_g28.find('Y') != std::string::npos ||
                     after_g28.find('y') != std::string::npos;
        bool has_z = after_g28.find('Z') != std::string::npos ||
                     after_g28.find('z') != std::string::npos;

        // If no specific axis mentioned, home all
        bool home_all = !has_x && !has_y && !has_z;

        {
            std::lock_guard<std::mutex> lock(homed_axes_mutex_);

            if (home_all) {
                // Home all axes
                homed_axes_ = "xyz";
                pos_x_.store(0.0);
                pos_y_.store(0.0);
                pos_z_.store(0.0);
                spdlog::info("[MoonrakerClientMock] Homed all axes (G28), homed_axes='xyz'");
            } else {
                // Home specific axes and update position
                if (has_x) {
                    if (homed_axes_.find('x') == std::string::npos) {
                        homed_axes_ += 'x';
                    }
                    pos_x_.store(0.0);
                }
                if (has_y) {
                    if (homed_axes_.find('y') == std::string::npos) {
                        homed_axes_ += 'y';
                    }
                    pos_y_.store(0.0);
                }
                if (has_z) {
                    if (homed_axes_.find('z') == std::string::npos) {
                        homed_axes_ += 'z';
                    }
                    pos_z_.store(0.0);
                }
                spdlog::info("[MoonrakerClientMock] Homed axes: X={} Y={} Z={}, homed_axes='{}'",
                             has_x, has_y, has_z, homed_axes_);
            }
        }
    }

    // Parse movement commands (G0/G1)
    // G0 X100 Y50 Z10 - Rapid move
    // G1 X100 Y50 Z10 E5 F3000 - Linear move (E and F ignored for now)
    if (gcode.find("G0") != std::string::npos || gcode.find("G1") != std::string::npos) {
        bool is_relative = relative_mode_.load();

        // Helper lambda to parse axis value from gcode string
        auto parse_axis = [&gcode](char axis) -> std::pair<bool, double> {
            // Look for the axis letter followed by a number
            size_t pos = gcode.find(axis);
            if (pos == std::string::npos) {
                // Try lowercase
                pos = gcode.find(static_cast<char>(axis + 32));
            }
            if (pos != std::string::npos && pos + 1 < gcode.length()) {
                // Skip any spaces after the axis letter
                size_t value_start = pos + 1;
                while (value_start < gcode.length() && gcode[value_start] == ' ') {
                    value_start++;
                }
                if (value_start < gcode.length()) {
                    try {
                        double value = std::stod(gcode.substr(value_start));
                        return {true, value};
                    } catch (...) {
                        // Parse error, ignore this axis
                    }
                }
            }
            return {false, 0.0};
        };

        auto [has_x, x_val] = parse_axis('X');
        auto [has_y, y_val] = parse_axis('Y');
        auto [has_z, z_val] = parse_axis('Z');

        if (has_x) {
            if (is_relative) {
                pos_x_.store(pos_x_.load() + x_val);
            } else {
                pos_x_.store(x_val);
            }
        }
        if (has_y) {
            if (is_relative) {
                pos_y_.store(pos_y_.load() + y_val);
            } else {
                pos_y_.store(y_val);
            }
        }
        if (has_z) {
            if (is_relative) {
                pos_z_.store(pos_z_.load() + z_val);
            } else {
                pos_z_.store(z_val);
            }
        }

        if (has_x || has_y || has_z) {
            spdlog::debug("[MoonrakerClientMock] Move {} X={} Y={} Z={} (mode={})",
                          gcode.find("G0") != std::string::npos ? "G0" : "G1", pos_x_.load(),
                          pos_y_.load(), pos_z_.load(), is_relative ? "relative" : "absolute");
        }
    }

    // Parse print job commands
    // SDCARD_PRINT_FILE FILENAME=xxx - Start printing a file
    if (gcode.find("SDCARD_PRINT_FILE") != std::string::npos) {
        size_t filename_pos = gcode.find("FILENAME=");
        if (filename_pos != std::string::npos) {
            // Extract filename (ends at space or end of string)
            size_t start = filename_pos + 9;
            size_t end = gcode.find(' ', start);
            std::string filename =
                (end != std::string::npos) ? gcode.substr(start, end - start) : gcode.substr(start);

            {
                std::lock_guard<std::mutex> lock(print_mutex_);
                print_filename_ = filename;
            }
            print_state_.store(1); // printing
            print_progress_.store(0.0);
            spdlog::info("[MoonrakerClientMock] Started print: {}", filename);
        }
    }
    // PAUSE - Pause current print
    else if (gcode == "PAUSE" || gcode.find("PAUSE ") == 0) {
        if (print_state_.load() == 1) { // Only pause if printing
            print_state_.store(2);      // paused
            spdlog::info("[MoonrakerClientMock] Print paused");
        }
    }
    // RESUME - Resume paused print
    else if (gcode == "RESUME" || gcode.find("RESUME ") == 0) {
        if (print_state_.load() == 2) { // Only resume if paused
            print_state_.store(1);      // printing
            spdlog::info("[MoonrakerClientMock] Print resumed");
        }
    }
    // CANCEL_PRINT - Cancel current print
    else if (gcode == "CANCEL_PRINT" || gcode.find("CANCEL_PRINT ") == 0) {
        print_state_.store(4); // cancelled
        spdlog::info("[MoonrakerClientMock] Print cancelled");
        // Note: Transition to standby is handled in simulation loop after brief delay
    }
    // M112 - Emergency stop
    else if (gcode.find("M112") != std::string::npos) {
        print_state_.store(5); // error
        spdlog::warn("[MoonrakerClientMock] Emergency stop (M112)!");
    }

    // ========================================================================
    // UNIMPLEMENTED G-CODE STUBS - Log warnings for missing features
    // ========================================================================

    // Fan control (NOT IMPLEMENTED)
    if (gcode.find("M106") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: M106 (fan speed) NOT IMPLEMENTED - fan_speed_ unchanged");
    } else if (gcode.find("M107") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: M107 (fan off) NOT IMPLEMENTED - fan_speed_ unchanged");
    } else if (gcode.find("SET_FAN_SPEED") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: SET_FAN_SPEED NOT IMPLEMENTED - fan_speed_ unchanged");
    }

    // Extrusion control (NOT IMPLEMENTED)
    if (gcode.find("G92") != std::string::npos && gcode.find('E') != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: G92 E (set extruder position) NOT IMPLEMENTED");
    }
    if ((gcode.find("G0") != std::string::npos || gcode.find("G1") != std::string::npos) &&
        gcode.find('E') != std::string::npos) {
        spdlog::debug("[MoonrakerClientMock] Note: Extrusion (E parameter) ignored in G0/G1");
    }

    // Bed mesh (NOT IMPLEMENTED)
    if (gcode.find("BED_MESH_CALIBRATE") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: BED_MESH_CALIBRATE NOT IMPLEMENTED");
    } else if (gcode.find("BED_MESH_PROFILE") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: BED_MESH_PROFILE NOT IMPLEMENTED");
    } else if (gcode.find("BED_MESH_CLEAR") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: BED_MESH_CLEAR NOT IMPLEMENTED");
    }

    // Z offset (NOT IMPLEMENTED)
    if (gcode.find("SET_GCODE_OFFSET") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: SET_GCODE_OFFSET NOT IMPLEMENTED");
    }

    // Input shaping (NOT IMPLEMENTED)
    if (gcode.find("SET_INPUT_SHAPER") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: SET_INPUT_SHAPER NOT IMPLEMENTED");
    }

    // Pressure advance (NOT IMPLEMENTED)
    if (gcode.find("SET_PRESSURE_ADVANCE") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: SET_PRESSURE_ADVANCE NOT IMPLEMENTED");
    }

    // LED control (NOT IMPLEMENTED)
    if (gcode.find("SET_LED") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: SET_LED NOT IMPLEMENTED");
    }

    // Firmware restart (NOT IMPLEMENTED)
    if (gcode.find("FIRMWARE_RESTART") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: FIRMWARE_RESTART NOT IMPLEMENTED");
    } else if (gcode.find("RESTART") != std::string::npos && gcode.find("FIRMWARE") == std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: RESTART NOT IMPLEMENTED");
    }

    // QGL / Z-tilt (NOT IMPLEMENTED)
    if (gcode.find("QUAD_GANTRY_LEVEL") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: QUAD_GANTRY_LEVEL NOT IMPLEMENTED");
    } else if (gcode.find("Z_TILT_ADJUST") != std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: Z_TILT_ADJUST NOT IMPLEMENTED");
    }

    // Probe (NOT IMPLEMENTED)
    if (gcode.find("PROBE") != std::string::npos && gcode.find("BED_MESH") == std::string::npos) {
        spdlog::warn("[MoonrakerClientMock] STUB: PROBE NOT IMPLEMENTED");
    }

    return 0; // Success
}

std::string MoonrakerClientMock::get_print_state_string() const {
    switch (print_state_.load()) {
    case 0:
        return "standby";
    case 1:
        return "printing";
    case 2:
        return "paused";
    case 3:
        return "complete";
    case 4:
        return "cancelled";
    case 5:
        return "error";
    default:
        return "standby";
    }
}

// ============================================================================
// Temperature Simulation
// ============================================================================

void MoonrakerClientMock::dispatch_initial_state() {
    // Build initial state JSON matching real Moonraker subscription response format
    // Uses current simulated values (room temp by default, or preset values if set)
    double ext_temp = extruder_temp_.load();
    double ext_target = extruder_target_.load();
    double bed_temp_val = bed_temp_.load();
    double bed_target_val = bed_target_.load();
    double x = pos_x_.load();
    double y = pos_y_.load();
    double z = pos_z_.load();
    int speed = speed_factor_.load();
    int flow = flow_factor_.load();
    int fan = fan_speed_.load();

    // Get homed_axes with thread safety
    std::string homed;
    {
        std::lock_guard<std::mutex> lock(homed_axes_mutex_);
        homed = homed_axes_;
    }

    // Get print state with thread safety
    std::string print_state_str = get_print_state_string();
    std::string filename;
    {
        std::lock_guard<std::mutex> lock(print_mutex_);
        filename = print_filename_;
    }
    double progress = print_progress_.load();

    json initial_status = {
        {"extruder", {
            {"temperature", ext_temp},
            {"target", ext_target}
        }},
        {"heater_bed", {
            {"temperature", bed_temp_val},
            {"target", bed_target_val}
        }},
        {"toolhead", {
            {"position", {x, y, z, 0.0}},
            {"homed_axes", homed}
        }},
        {"gcode_move", {
            {"speed_factor", speed / 100.0},
            {"extrude_factor", flow / 100.0}
        }},
        {"fan", {
            {"speed", fan / 255.0}
        }},
        {"print_stats", {
            {"state", print_state_str},
            {"filename", filename}
        }},
        {"virtual_sdcard", {
            {"progress", progress}
        }}
    };

    spdlog::info("[MoonrakerClientMock] Dispatching initial state: extruder={}/{}°C, bed={}/{}°C, homed_axes='{}'",
                 ext_temp, ext_target, bed_temp_val, bed_target_val, homed);

    // Use the base class dispatch method (same as real client)
    dispatch_status_update(initial_status);
}

void MoonrakerClientMock::set_extruder_target(double target) {
    extruder_target_.store(target);
}

void MoonrakerClientMock::set_bed_target(double target) {
    bed_target_.store(target);
}

void MoonrakerClientMock::start_temperature_simulation() {
    if (simulation_running_.load()) {
        return; // Already running
    }

    simulation_running_.store(true);
    simulation_thread_ = std::thread(&MoonrakerClientMock::temperature_simulation_loop, this);
    spdlog::info("[MoonrakerClientMock] Temperature simulation started");
}

void MoonrakerClientMock::stop_temperature_simulation() {
    if (!simulation_running_.load()) {
        return; // Not running
    }

    simulation_running_.store(false);
    if (simulation_thread_.joinable()) {
        simulation_thread_.join();
    }
    spdlog::info("[MoonrakerClientMock] Temperature simulation stopped");
}

void MoonrakerClientMock::temperature_simulation_loop() {
    const double dt = SIMULATION_INTERVAL_MS / 1000.0; // Convert to seconds

    while (simulation_running_.load()) {
        uint32_t tick = tick_count_.fetch_add(1);

        // Get current temperature state
        double ext_temp = extruder_temp_.load();
        double ext_target = extruder_target_.load();
        double bed_temp_val = bed_temp_.load();
        double bed_target_val = bed_target_.load();

        // Simulate extruder temperature change
        if (ext_target > 0) {
            if (ext_temp < ext_target) {
                ext_temp += EXTRUDER_HEAT_RATE * dt;
                if (ext_temp > ext_target) ext_temp = ext_target;
            } else if (ext_temp > ext_target) {
                ext_temp -= EXTRUDER_COOL_RATE * dt;
                if (ext_temp < ext_target) ext_temp = ext_target;
            }
        } else {
            if (ext_temp > ROOM_TEMP) {
                ext_temp -= EXTRUDER_COOL_RATE * dt;
                if (ext_temp < ROOM_TEMP) ext_temp = ROOM_TEMP;
            }
        }
        extruder_temp_.store(ext_temp);

        // Simulate bed temperature change
        if (bed_target_val > 0) {
            if (bed_temp_val < bed_target_val) {
                bed_temp_val += BED_HEAT_RATE * dt;
                if (bed_temp_val > bed_target_val) bed_temp_val = bed_target_val;
            } else if (bed_temp_val > bed_target_val) {
                bed_temp_val -= BED_COOL_RATE * dt;
                if (bed_temp_val < bed_target_val) bed_temp_val = bed_target_val;
            }
        } else {
            if (bed_temp_val > ROOM_TEMP) {
                bed_temp_val -= BED_COOL_RATE * dt;
                if (bed_temp_val < ROOM_TEMP) bed_temp_val = ROOM_TEMP;
            }
        }
        bed_temp_.store(bed_temp_val);

        // Get current position (set by G-code commands, not auto-simulated)
        double x = pos_x_.load();
        double y = pos_y_.load();
        double z = pos_z_.load();

        // Get homed_axes with thread safety
        std::string homed;
        {
            std::lock_guard<std::mutex> lock(homed_axes_mutex_);
            homed = homed_axes_;
        }

        // Simulate speed/flow oscillation (90-110%)
        int speed = 100 + static_cast<int>(10.0 * std::sin(tick / 20.0));
        int flow = 100 + static_cast<int>(5.0 * std::cos(tick / 30.0));
        speed_factor_.store(speed);
        flow_factor_.store(flow);

        // Simulate fan ramping up (0-255 over 60 ticks)
        int fan = std::min(255, static_cast<int>((tick / 60.0) * 255.0));
        fan_speed_.store(fan);

        // Simulate print progress
        int current_print_state = print_state_.load();
        double progress = print_progress_.load();

        // Static counter for cancelled->standby transition delay
        static int cancelled_ticks = 0;

        if (current_print_state == 1) { // printing
            // Increment progress by small amount each tick (complete in ~100 ticks = 50 seconds)
            progress += 0.01;
            if (progress >= 1.0) {
                progress = 1.0;
                print_state_.store(3); // complete
                spdlog::info("[MoonrakerClientMock] Print complete");
            }
            print_progress_.store(progress);
        } else if (current_print_state == 4) { // cancelled
            // Transition to standby after 2 ticks (1 second)
            cancelled_ticks++;
            if (cancelled_ticks >= 2) {
                print_state_.store(0); // standby
                {
                    std::lock_guard<std::mutex> lock(print_mutex_);
                    print_filename_.clear();
                }
                print_progress_.store(0.0);
                cancelled_ticks = 0;
                spdlog::info("[MoonrakerClientMock] Print state reset to standby after cancel");
            }
        } else {
            cancelled_ticks = 0; // Reset counter if not in cancelled state
        }

        // Get print state string and filename with thread safety
        std::string print_state_str = get_print_state_string();
        std::string filename;
        {
            std::lock_guard<std::mutex> lock(print_mutex_);
            filename = print_filename_;
        }

        // Build notification JSON (same format as real Moonraker)
        // Real Moonraker sends: {"params": [status_object, eventtime]}
        json status_obj = {
            {"extruder", {
                {"temperature", ext_temp},
                {"target", ext_target}
            }},
            {"heater_bed", {
                {"temperature", bed_temp_val},
                {"target", bed_target_val}
            }},
            {"toolhead", {
                {"position", {x, y, z, 0.0}},
                {"homed_axes", homed}
            }},
            {"gcode_move", {
                {"speed_factor", speed / 100.0},
                {"extrude_factor", flow / 100.0}
            }},
            {"fan", {
                {"speed", fan / 255.0}
            }},
            {"print_stats", {
                {"state", print_state_str},
                {"filename", filename}
            }},
            {"virtual_sdcard", {
                {"progress", print_progress_.load()}
            }}
        };
        json notification = {
            {"method", "notify_status_update"},
            {"params", json::array({status_obj, tick * dt})}  // [status, eventtime]
        };

        // Push notification through all registered callbacks
        std::vector<std::function<void(json)>> callbacks_copy;
        {
            std::lock_guard<std::mutex> lock(callbacks_mutex_);
            callbacks_copy = notify_callbacks_;
        }
        for (const auto& cb : callbacks_copy) {
            if (cb) {
                cb(notification);
            }
        }

        // Sleep until next update
        std::this_thread::sleep_for(std::chrono::milliseconds(SIMULATION_INTERVAL_MS));
    }
}
