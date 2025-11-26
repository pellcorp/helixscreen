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

#ifndef MOONRAKER_CLIENT_MOCK_H
#define MOONRAKER_CLIENT_MOCK_H

#include "moonraker_client.h"

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief Mock Moonraker client for testing without real printer connection
 *
 * Simulates printer hardware discovery with configurable test data.
 * Useful for UI development and testing without physical hardware.
 *
 * Inherits from MoonrakerClient to provide drop-in replacement compatibility.
 * Overrides discover_printer() to populate test data without WebSocket connection.
 */
class MoonrakerClientMock : public MoonrakerClient {
  public:
    enum class PrinterType {
        VORON_24,           // Voron 2.4 (CoreXY, chamber heating)
        VORON_TRIDENT,      // Voron Trident (3Z, CoreXY)
        CREALITY_K1,        // Creality K1/K1 Max (bed slinger style)
        FLASHFORGE_AD5M,    // FlashForge Adventurer 5M (enclosed)
        GENERIC_COREXY,     // Generic CoreXY printer
        GENERIC_BEDSLINGER, // Generic i3-style printer
        MULTI_EXTRUDER      // Multi-extruder test case (2 extruders)
    };

    MoonrakerClientMock(PrinterType type = PrinterType::VORON_24);
    ~MoonrakerClientMock();

    // Prevent copying (has thread state)
    MoonrakerClientMock(const MoonrakerClientMock&) = delete;
    MoonrakerClientMock& operator=(const MoonrakerClientMock&) = delete;

    /**
     * @brief Simulate WebSocket connection (no real network I/O)
     *
     * Overrides base class to simulate successful connection without
     * actual WebSocket establishment. Immediately invokes on_connected callback.
     *
     * @param url WebSocket URL (ignored in mock)
     * @param on_connected Callback invoked immediately
     * @param on_disconnected Callback stored but never invoked in mock
     * @return Always returns 0 (success)
     */
    int connect(const char* url, std::function<void()> on_connected,
                std::function<void()> on_disconnected) override;

    /**
     * @brief Simulate printer hardware discovery
     *
     * Overrides base class method to immediately populate hardware lists
     * based on configured printer type and invoke completion callback.
     *
     * @param on_complete Callback invoked after discovery completes
     */
    void discover_printer(std::function<void()> on_complete) override;

    /**
     * @brief Simulate WebSocket disconnection (no real network I/O)
     *
     * Overrides base class to simulate disconnection without actual WebSocket teardown.
     */
    void disconnect() override;

    /**
     * @brief Simulate JSON-RPC request without parameters
     *
     * Overrides base class to log and return success without network I/O.
     *
     * @param method RPC method name
     * @return Always returns 0 (success)
     */
    int send_jsonrpc(const std::string& method) override;

    /**
     * @brief Simulate JSON-RPC request with parameters
     *
     * Overrides base class to log and return success without network I/O.
     *
     * @param method RPC method name
     * @param params JSON parameters object (ignored in mock)
     * @return Always returns 0 (success)
     */
    int send_jsonrpc(const std::string& method, const json& params) override;

    /**
     * @brief Simulate JSON-RPC request with callback
     *
     * Overrides base class to log and return success without network I/O.
     *
     * @param method RPC method name
     * @param params JSON parameters object (ignored in mock)
     * @param cb Callback function (not invoked in mock)
     * @return Always returns 0 (success)
     */
    int send_jsonrpc(const std::string& method, const json& params,
                     std::function<void(json)> cb) override;

    /**
     * @brief Simulate JSON-RPC request with success/error callbacks
     *
     * Overrides base class to log and return success without network I/O.
     *
     * @param method RPC method name
     * @param params JSON parameters object (ignored in mock)
     * @param success_cb Success callback (not invoked in mock)
     * @param error_cb Error callback (not invoked in mock)
     * @param timeout_ms Timeout (ignored in mock)
     * @return Always returns 0 (success)
     */
    int send_jsonrpc(const std::string& method, const json& params,
                     std::function<void(json)> success_cb,
                     std::function<void(const MoonrakerError&)> error_cb,
                     uint32_t timeout_ms = 0) override;

    /**
     * @brief Simulate G-code script command
     *
     * Overrides base class to log and return success without network I/O.
     *
     * @param gcode G-code string
     * @return Always returns 0 (success)
     */
    int gcode_script(const std::string& gcode) override;

    /**
     * @brief Set printer type for mock data generation
     *
     * @param type Printer type to simulate
     */
    void set_printer_type(PrinterType type) {
        printer_type_ = type;
    }

    /**
     * @brief Start temperature simulation loop
     *
     * Begins a background thread that simulates temperature changes
     * and pushes updates via notify_status_update callback.
     * Called automatically on connect().
     */
    void start_temperature_simulation();

    /**
     * @brief Stop temperature simulation loop
     *
     * Stops the background simulation thread.
     * Called automatically on disconnect() and destructor.
     */
    void stop_temperature_simulation();

    /**
     * @brief Set simulated extruder target temperature
     *
     * Starts heating/cooling simulation toward target.
     *
     * @param target Target temperature in Celsius
     */
    void set_extruder_target(double target);

    /**
     * @brief Set simulated bed target temperature
     *
     * Starts heating/cooling simulation toward target.
     *
     * @param target Target temperature in Celsius
     */
    void set_bed_target(double target);

  private:
    /**
     * @brief Populate hardware lists based on configured printer type
     *
     * Directly modifies the protected member variables inherited from
     * MoonrakerClient (heaters_, sensors_, fans_, leds_).
     */
    void populate_hardware();

    /**
     * @brief Generate synthetic bed mesh data for testing
     *
     * Creates a realistic dome-shaped mesh (7×7 points, 0-0.3mm Z range).
     * Populates active_bed_mesh_ with test data compatible with renderer.
     */
    void generate_mock_bed_mesh();

    /**
     * @brief Temperature simulation loop (runs in background thread)
     */
    void temperature_simulation_loop();

    /**
     * @brief Dispatch initial printer state to observers
     *
     * Called during connect() to send initial state, matching the behavior
     * of the real MoonrakerClient which sends initial state from the
     * subscription response. Uses dispatch_status_update() from base class.
     */
    void dispatch_initial_state();

    /**
     * @brief Get print state as string for Moonraker-compatible notifications
     *
     * @return String representation: "standby", "printing", "paused", "complete", "cancelled", "error"
     */
    std::string get_print_state_string() const;

  private:
    PrinterType printer_type_;

    // Temperature simulation state
    std::atomic<double> extruder_temp_{25.0};   // Current temperature
    std::atomic<double> extruder_target_{0.0};  // Target temperature (0 = off)
    std::atomic<double> bed_temp_{25.0};        // Current temperature
    std::atomic<double> bed_target_{0.0};       // Target temperature (0 = off)

    // Position simulation state
    std::atomic<double> pos_x_{0.0};
    std::atomic<double> pos_y_{0.0};
    std::atomic<double> pos_z_{0.0};

    // Motion mode state
    std::atomic<bool> relative_mode_{false};  // G90=absolute (false), G91=relative (true)

    // Homing state (needs mutex since std::string is not atomic)
    mutable std::mutex homed_axes_mutex_;
    std::string homed_axes_;

    // Print simulation state
    std::atomic<int> print_state_{0};           // 0=standby, 1=printing, 2=paused, 3=complete, 4=cancelled, 5=error
    std::string print_filename_;                // Current print file (protected by print_mutex_)
    mutable std::mutex print_mutex_;            // Protects print_filename_
    std::atomic<double> print_progress_{0.0};   // 0.0 to 1.0
    std::atomic<int> speed_factor_{100};        // Percentage
    std::atomic<int> flow_factor_{100};         // Percentage
    std::atomic<int> fan_speed_{0};             // 0-255

    // Simulation tick counter
    std::atomic<uint32_t> tick_count_{0};

    // Simulation thread control
    std::thread simulation_thread_;
    std::atomic<bool> simulation_running_{false};

    // Simulation parameters (realistic heating rates)
    static constexpr double ROOM_TEMP = 25.0;
    static constexpr double EXTRUDER_HEAT_RATE = 3.0;  // °C/sec when heating
    static constexpr double EXTRUDER_COOL_RATE = 1.5;  // °C/sec when cooling
    static constexpr double BED_HEAT_RATE = 1.0;       // °C/sec when heating
    static constexpr double BED_COOL_RATE = 0.3;       // °C/sec when cooling
    static constexpr int SIMULATION_INTERVAL_MS = 500; // Update frequency
};

#endif // MOONRAKER_CLIENT_MOCK_H
