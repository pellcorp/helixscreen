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

#pragma once

#include <string>
#include <functional>
#include <map>

#ifndef __APPLE__
// ============================================================================
// Linux Implementation: Full wpa_supplicant integration
// ============================================================================

#include "hv/hloop.h"
#include "hv/EventLoop.h"
#include "hv/EventLoopThread.h"

// Forward declaration - avoid including wpa_ctrl.h in header
struct wpa_ctrl;

/**
 * @brief wpa_supplicant backend using libhv async event loop
 *
 * Provides asynchronous communication with wpa_supplicant daemon via
 * Unix socket control interface. Uses libhv's EventLoopThread for
 * non-blocking socket I/O.
 *
 * Architecture:
 * - Inherits privately from hv::EventLoopThread for async I/O
 * - Dual wpa_ctrl connections: control (commands) + monitor (events)
 * - Event callbacks broadcast to registered handlers
 * - Commands sent synchronously via wpa_ctrl_request()
 *
 * Usage:
 * @code
 *   WifiBackendWpa backend;
 *   backend.register_callback("scan", [](const std::string& event) {
 *       // Handle scan complete events
 *   });
 *   backend.start();  // Connects to wpa_supplicant, starts event loop
 *   std::string result = backend.send_command("SCAN");
 *   backend.stop();   // Clean shutdown
 * @endcode
 */
class WifiBackendWpa : private hv::EventLoopThread {
public:
    /**
     * @brief Construct WiFi backend
     *
     * Does NOT connect to wpa_supplicant. Call start() to initialize.
     */
    WifiBackendWpa();

    /**
     * @brief Destructor - ensures clean shutdown
     */
    ~WifiBackendWpa();

    /**
     * @brief Start wpa_supplicant connection and event loop
     *
     * Discovers wpa_supplicant socket, establishes dual connections
     * (control + monitor), and starts libhv event loop thread.
     *
     * Socket discovery order:
     * 1. /run/wpa_supplicant/wlan0 (modern systemd)
     * 2. /var/run/wpa_supplicant/wlan0 (older systems)
     * 3. Auto-detect first non-P2P socket in directory
     *
     * Thread-safe: Can be called multiple times (idempotent if already running)
     */
    void start();

    /**
     * @brief Stop event loop and disconnect from wpa_supplicant
     *
     * Blocks until event loop thread terminates.
     */
    void stop();

    /**
     * @brief Register callback for wpa_supplicant events
     *
     * Events are broadcast to ALL registered callbacks asynchronously
     * from the libhv event loop thread. Ensure thread safety in handlers.
     *
     * Common event prefixes:
     * - "CTRL-EVENT-SCAN-RESULTS" - Scan complete
     * - "CTRL-EVENT-CONNECTED" - Network connected
     * - "CTRL-EVENT-DISCONNECTED" - Network disconnected
     * - "WPS-" - WPS events
     *
     * @param name Identifier for this callback (for future removal/replacement)
     * @param callback Handler function receiving event string
     */
    void register_callback(const std::string& name,
                          std::function<void(const std::string&)> callback);

    /**
     * @brief Send synchronous command to wpa_supplicant
     *
     * Blocks until response received or timeout (usually <100ms).
     *
     * Common commands:
     * - "SCAN" - Trigger network scan
     * - "SCAN_RESULTS" - Get scan results (tab-separated format)
     * - "ADD_NETWORK" - Add network configuration (returns network ID)
     * - "SET_NETWORK <id> ssid \"<ssid>\"" - Set network SSID
     * - "SET_NETWORK <id> psk \"<password>\"" - Set WPA password
     * - "ENABLE_NETWORK <id>" - Connect to network
     * - "STATUS" - Get connection status
     *
     * @param cmd Command string (see wpa_supplicant control interface docs)
     * @return Response string (may contain newlines), or empty on error
     */
    std::string send_command(const std::string& cmd);

private:
    /**
     * @brief Initialize wpa_supplicant connection (runs in event loop thread)
     *
     * Called by start() in the context of the libhv event loop thread.
     * Discovers socket, opens connections, registers I/O callbacks.
     */
    void init_wpa();

    /**
     * @brief Handle incoming wpa_supplicant events
     *
     * Broadcasts event to all registered callbacks.
     *
     * @param data Raw event data from wpa_supplicant
     * @param len Length of event data in bytes
     */
    void handle_wpa_events(void* data, int len);

    /**
     * @brief Static trampoline for C callback compatibility
     *
     * libhv uses C-style function pointers for I/O callbacks.
     * This static method extracts the instance pointer from hio_context()
     * and forwards to the member function handle_wpa_events().
     *
     * @param io libhv I/O handle
     * @param data Event data buffer
     * @param readbyte Number of bytes read
     */
    static void _handle_wpa_events(hio_t* io, void* data, int readbyte);

    struct wpa_ctrl* conn;  ///< Control connection for sending commands
    std::map<std::string, std::function<void(const std::string&)>> callbacks;  ///< Registered event handlers
};

#else
// ============================================================================
// macOS Stub Implementation: Mock mode for simulator testing
// ============================================================================

/**
 * @brief Stub WiFi backend for macOS simulator
 *
 * Provides no-op implementations. All operations log at debug level
 * and return empty/default values.
 */
class WifiBackendWpa {
public:
    WifiBackendWpa() = default;
    ~WifiBackendWpa() = default;

    void start() {}
    void stop() {}
    void register_callback(const std::string&, std::function<void(const std::string&)>) {}
    std::string send_command(const std::string&) { return ""; }
};

#endif // __APPLE__
