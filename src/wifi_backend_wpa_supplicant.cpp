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

#include "wifi_backend_wpa.h"
#include "spdlog/spdlog.h"

#ifndef __APPLE__
// ============================================================================
// Linux Implementation: Full wpa_supplicant integration
// ============================================================================

#include "wpa_ctrl.h"
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;

WifiBackendWpa::WifiBackendWpa()
    : hv::EventLoopThread(NULL)
    , conn(NULL)
{
    spdlog::debug("[WifiBackend] Initialized (wpa_supplicant mode)");
}

WifiBackendWpa::~WifiBackendWpa() {
    spdlog::trace("[WifiBackend] Destructor called");
    stop();
}

void WifiBackendWpa::start() {
    if (isRunning()) {
        // Event loop already running - schedule initialization in loop thread
        spdlog::debug("[WifiBackend] Already running, scheduling init_wpa in loop");
        loop()->runInLoop(std::bind(&WifiBackendWpa::init_wpa, this));
    } else {
        // Start new event loop thread with initialization callback
        spdlog::info("[WifiBackend] Starting event loop thread");
        hv::EventLoopThread::start(true, [this]() {
            WifiBackendWpa::init_wpa();
            return 0;
        });
    }
}

void WifiBackendWpa::stop() {
    if (!isRunning()) {
        spdlog::trace("[WifiBackend] Not running, nothing to stop");
        return;
    }

    spdlog::info("[WifiBackend] Stopping event loop thread");
    hv::EventLoopThread::stop(true);  // Block until thread terminates
    spdlog::trace("[WifiBackend] Event loop stopped");
}

void WifiBackendWpa::register_callback(const std::string& name,
                                       std::function<void(const std::string&)> callback) {
    const auto& entry = callbacks.find(name);
    if (entry == callbacks.end()) {
        callbacks.insert({name, callback});
        spdlog::debug("[WifiBackend] Registered callback '{}'", name);
    } else {
        // Callback already exists - could replace it, but parent doesn't
        spdlog::warn("[WifiBackend] Callback '{}' already registered (not replacing)", name);
    }
}

void WifiBackendWpa::init_wpa() {
    spdlog::trace("[WifiBackend] init_wpa() called in event loop thread");

    // Socket discovery: Try common paths
    std::string wpa_socket;
    bool socket_found = false;

    // Try modern systemd path first: /run/wpa_supplicant
    std::string base_path = "/run/wpa_supplicant";
    if (fs::exists(base_path) && fs::is_directory(base_path)) {
        spdlog::debug("[WifiBackend] Searching for wpa_supplicant socket in {}", base_path);

        for (const auto& entry : fs::directory_iterator(base_path)) {
            if (fs::is_socket(entry.path())) {
                std::string socket_path = entry.path().string();

                // Filter out P2P sockets (e.g., p2p-dev-wlan0)
                if (socket_path.find("p2p") == std::string::npos) {
                    wpa_socket = socket_path;
                    socket_found = true;
                    spdlog::info("[WifiBackend] Found wpa_supplicant socket: {}", wpa_socket);
                    break;
                }
            }
        }
    }

    // Try older path if not found: /var/run/wpa_supplicant
    if (!socket_found) {
        base_path = "/var/run/wpa_supplicant";
        if (fs::exists(base_path) && fs::is_directory(base_path)) {
            spdlog::debug("[WifiBackend] Searching for wpa_supplicant socket in {}", base_path);

            for (const auto& entry : fs::directory_iterator(base_path)) {
                if (fs::is_socket(entry.path())) {
                    std::string socket_path = entry.path().string();

                    // Filter out P2P sockets
                    if (socket_path.find("p2p") == std::string::npos) {
                        wpa_socket = socket_path;
                        socket_found = true;
                        spdlog::info("[WifiBackend] Found wpa_supplicant socket: {}", wpa_socket);
                        break;
                    }
                }
            }
        }
    }

    if (!socket_found) {
        spdlog::error("[WifiBackend] Could not find wpa_supplicant socket in /run or /var/run");
        spdlog::error("[WifiBackend] Is wpa_supplicant daemon running?");
        return;
    }

    // Open control connection (for sending commands)
    if (conn == NULL) {
        conn = wpa_ctrl_open(wpa_socket.c_str());
        if (conn == NULL) {
            spdlog::error("[WifiBackend] Failed to open control connection to {}", wpa_socket);
            return;
        }
        spdlog::debug("[WifiBackend] Opened control connection");
    }

    // Open monitor connection (for receiving events)
    struct wpa_ctrl* mon_conn = wpa_ctrl_open(wpa_socket.c_str());
    if (mon_conn == NULL) {
        spdlog::error("[WifiBackend] Failed to open monitor connection to {}", wpa_socket);
        return;
    }

    // Attach to wpa_supplicant event stream
    if (wpa_ctrl_attach(mon_conn) != 0) {
        spdlog::error("[WifiBackend] Failed to attach to wpa_supplicant events");
        wpa_ctrl_close(mon_conn);
        return;
    }
    spdlog::info("[WifiBackend] Attached to wpa_supplicant event stream");

    // Get file descriptor for monitor socket
    int monfd = wpa_ctrl_get_fd(mon_conn);
    if (monfd < 0) {
        spdlog::error("[WifiBackend] Failed to get monitor socket file descriptor");
        wpa_ctrl_close(mon_conn);
        return;
    }
    spdlog::trace("[WifiBackend] Monitor socket fd: {}", monfd);

    // Register with libhv event loop for async I/O
    hio_t* io = hio_get(loop()->loop(), monfd);
    if (io == NULL) {
        spdlog::error("[WifiBackend] Failed to register monitor socket with libhv");
        wpa_ctrl_close(mon_conn);
        return;
    }

    // Set up I/O callbacks
    hio_set_context(io, this);  // Store 'this' pointer for static callback
    hio_setcb_read(io, WifiBackendWpa::_handle_wpa_events);  // Static trampoline
    hio_read_start(io);  // Start monitoring socket for events

    spdlog::info("[WifiBackend] wpa_supplicant backend initialized successfully");
}

void WifiBackendWpa::handle_wpa_events(void* data, int len) {
    if (data == nullptr || len <= 0) {
        spdlog::warn("[WifiBackend] Received empty event");
        return;
    }

    // Convert to string (may contain newlines)
    std::string event = std::string(static_cast<char*>(data), len);

    spdlog::trace("[WifiBackend] Event received: {}", event);

    // Broadcast to ALL registered callbacks
    for (const auto& entry : callbacks) {
        spdlog::trace("[WifiBackend] Dispatching event to callback '{}'", entry.first);
        entry.second(event);
    }
}

void WifiBackendWpa::_handle_wpa_events(hio_t* io, void* data, int readbyte) {
    // Static trampoline: Extract instance pointer and forward to member function
    WifiBackendWpa* instance = static_cast<WifiBackendWpa*>(hio_context(io));
    if (instance) {
        instance->handle_wpa_events(data, readbyte);
    } else {
        spdlog::error("[WifiBackend] Static callback invoked with NULL context");
    }
}

std::string WifiBackendWpa::send_command(const std::string& cmd) {
    if (conn == NULL) {
        spdlog::warn("[WifiBackend] send_command called but not connected to wpa_supplicant");
        return "";
    }

    char resp[4096];
    size_t len = sizeof(resp) - 1;

    spdlog::trace("[WifiBackend] Sending command: {}", cmd);

    int result = wpa_ctrl_request(conn, cmd.c_str(), cmd.length(), resp, &len, NULL);
    if (result != 0) {
        spdlog::error("[WifiBackend] Command failed: {} (error code: {})", cmd, result);
        return "";
    }

    // Null-terminate response
    resp[len] = '\0';

    spdlog::trace("[WifiBackend] Command response ({} bytes): {}", len, std::string(resp, len));

    return std::string(resp, len);
}

#else
// ============================================================================
// macOS Stub Implementation: No-op for simulator
// ============================================================================

// Empty file - all methods are inline in header

#endif // __APPLE__
