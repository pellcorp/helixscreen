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

#include "ui_wizard_connection.h"
#include "ui_wizard.h"  // For ui_wizard_set_next_button_enabled()
#include "app_globals.h"
#include "moonraker_client.h"
#include "wizard_validation.h"
#include "config.h"
#include "lvgl/lvgl.h"
#include <spdlog/spdlog.h>
#include <string>
#include <cstring>

// ============================================================================
// Static Data & Subjects
// ============================================================================

// Subject declarations (module scope)
static lv_subject_t connection_ip;
static lv_subject_t connection_port;
static lv_subject_t connection_status;
static lv_subject_t connection_testing;  // 0=idle, 1=testing (controls spinner)

// String buffers (must be persistent)
static char connection_ip_buffer[128];
static char connection_port_buffer[8];
static char connection_status_buffer[256];

// Connection screen instance
static lv_obj_t* connection_screen_root = nullptr;

// Track whether connection has been validated
static bool connection_validated = false;

// ============================================================================
// Forward Declarations
// ============================================================================

static void on_test_connection_clicked(lv_event_t* e);
static void on_ip_input_changed(lv_event_t* e);
static void on_port_input_changed(lv_event_t* e);

// ============================================================================
// Subject Initialization
// ============================================================================

void ui_wizard_connection_init_subjects() {
    spdlog::debug("[Wizard Connection] Initializing subjects");

    // Load existing values from config if available
    Config* config = Config::get_instance();
    std::string default_ip = "";
    std::string default_port = "7125";  // Default Moonraker port

    try {
        default_ip = config->get<std::string>("/moonraker/host", "");
        int port_num = config->get<int>("/moonraker/port", 7125);
        default_port = std::to_string(port_num);
        spdlog::debug("[Wizard Connection] Loaded from config: {}:{}", default_ip, default_port);
    } catch (const std::exception& e) {
        spdlog::debug("[Wizard Connection] No existing config, using defaults");
    }

    // Initialize with values from config or defaults
    strncpy(connection_ip_buffer, default_ip.c_str(), sizeof(connection_ip_buffer) - 1);
    connection_ip_buffer[sizeof(connection_ip_buffer) - 1] = '\0';

    strncpy(connection_port_buffer, default_port.c_str(), sizeof(connection_port_buffer) - 1);
    connection_port_buffer[sizeof(connection_port_buffer) - 1] = '\0';

    lv_subject_init_string(&connection_ip, connection_ip_buffer, nullptr,
                          sizeof(connection_ip_buffer), connection_ip_buffer);

    lv_subject_init_string(&connection_port, connection_port_buffer, nullptr,
                          sizeof(connection_port_buffer), connection_port_buffer);

    lv_subject_init_string(&connection_status, connection_status_buffer, nullptr,
                          sizeof(connection_status_buffer), "");

    lv_subject_init_int(&connection_testing, 0);  // Not testing initially

    // Register globally for XML binding
    lv_xml_register_subject(nullptr, "connection_ip", &connection_ip);
    lv_xml_register_subject(nullptr, "connection_port", &connection_port);
    lv_xml_register_subject(nullptr, "connection_status", &connection_status);
    lv_xml_register_subject(nullptr, "connection_testing", &connection_testing);

    // Reset validation state
    connection_validated = false;

    // Check if we have a saved configuration that might already be valid
    if (!default_ip.empty() && !default_port.empty()) {
        // We have saved values, but they haven't been tested yet this session
        spdlog::debug("[Wizard Connection] Have saved config, but needs validation");
    }

    // Disable Next button until connection is validated
    ui_wizard_set_next_button_enabled(false);

    spdlog::info("[Wizard Connection] Subjects initialized (IP: {}, Port: {})",
                 default_ip.empty() ? "<empty>" : default_ip, default_port);
}

// ============================================================================
// Event Handlers
// ============================================================================

/**
 * @brief Handle Test Connection button click
 *
 * Validates inputs, attempts WebSocket connection to Moonraker,
 * and updates status based on result.
 */
static void on_test_connection_clicked(lv_event_t* e) {
    (void)e;  // Unused parameter

    // Get values from subjects
    const char* ip = lv_subject_get_string(&connection_ip);
    const char* port_str = lv_subject_get_string(&connection_port);

    spdlog::debug("[Wizard Connection] Test connection clicked: {}:{}", ip, port_str);

    // Clear previous validation state
    connection_validated = false;

    // Validate inputs
    if (!ip || strlen(ip) == 0) {
        lv_subject_copy_string(&connection_status, "Please enter an IP address or hostname");
        spdlog::warn("[Wizard Connection] Empty IP address");
        return;
    }

    if (!is_valid_ip_or_hostname(ip)) {
        lv_subject_copy_string(&connection_status, "Invalid IP address or hostname");
        spdlog::warn("[Wizard Connection] Invalid IP/hostname: {}", ip);
        return;
    }

    if (!is_valid_port(port_str)) {
        lv_subject_copy_string(&connection_status, "Invalid port (must be 1-65535)");
        spdlog::warn("[Wizard Connection] Invalid port: {}", port_str);
        return;
    }

    // Update status to show testing
    lv_subject_copy_string(&connection_status, "Testing connection...");
    lv_subject_set_int(&connection_testing, 1);  // Show spinner

    // Build WebSocket URL
    char ws_url[256];
    snprintf(ws_url, sizeof(ws_url), "ws://%s:%s/websocket", ip, port_str);
    spdlog::info("[Wizard Connection] Testing connection to: {}", ws_url);

    // Get MoonrakerClient instance
    MoonrakerClient* client = get_moonraker_client();
    if (!client) {
        lv_subject_copy_string(&connection_status, "Error: Moonraker client not initialized");
        lv_subject_set_int(&connection_testing, 0);
        spdlog::error("[Wizard Connection] MoonrakerClient is nullptr");
        return;
    }

    // Store IP/port for potential config save (before async callback)
    static std::string saved_ip;
    static std::string saved_port;
    saved_ip = ip;
    saved_port = port_str;

    // Set shorter timeout for wizard testing (5 seconds)
    client->set_connection_timeout(5000);

    // Attempt connection
    int result = client->connect(ws_url,
        // On connected callback
        []() {
            spdlog::info("[Wizard Connection] Connection successful!");
            lv_subject_copy_string(&connection_status, "✓ Connection successful!");
            lv_subject_set_int(&connection_testing, 0);  // Hide spinner
            connection_validated = true;

            // Enable Next button now that connection is validated
            ui_wizard_set_next_button_enabled(true);

            // Save configuration to helixconfig.json
            Config* config = Config::get_instance();
            try {
                config->set("/moonraker/host", saved_ip);
                config->set("/moonraker/port", std::stoi(saved_port));
                config->save();
                spdlog::info("[Wizard Connection] Saved configuration: {}:{}", saved_ip, saved_port);
            } catch (const std::exception& e) {
                spdlog::error("[Wizard Connection] Failed to save config: {}", e.what());
            }

            // Disconnect after successful test (we're just testing, not maintaining connection)
            MoonrakerClient* client = get_moonraker_client();
            if (client) {
                client->close();
            }
        },
        // On disconnected callback
        []() {
            // Only show error if we never successfully connected
            MoonrakerClient* client = get_moonraker_client();
            if (client && client->get_connection_state() == ConnectionState::FAILED) {
                spdlog::error("[Wizard Connection] Connection failed");
                lv_subject_copy_string(&connection_status,
                    "✗ Connection failed. Check IP/port and try again.");
                lv_subject_set_int(&connection_testing, 0);  // Hide spinner
                connection_validated = false;

                // Disable Next button since connection failed
                ui_wizard_set_next_button_enabled(false);
            }
        }
    );

    if (result != 0) {
        spdlog::error("[Wizard Connection] Failed to initiate connection: {}", result);
        lv_subject_copy_string(&connection_status, "Error starting connection test");
        lv_subject_set_int(&connection_testing, 0);
    }
}

/**
 * @brief Handle IP input field changes
 *
 * Clear status message when user starts typing
 */
static void on_ip_input_changed(lv_event_t* e) {
    (void)e;

    // Clear any previous status message when user modifies input
    const char* current_status = lv_subject_get_string(&connection_status);
    if (current_status && strlen(current_status) > 0) {
        lv_subject_copy_string(&connection_status, "");
    }

    // Clear validation state when input changes
    connection_validated = false;

    // Disable Next button since input changed
    ui_wizard_set_next_button_enabled(false);
}

/**
 * @brief Handle port input field changes
 *
 * Clear status message when user starts typing
 */
static void on_port_input_changed(lv_event_t* e) {
    (void)e;

    // Clear any previous status message when user modifies input
    const char* current_status = lv_subject_get_string(&connection_status);
    if (current_status && strlen(current_status) > 0) {
        lv_subject_copy_string(&connection_status, "");
    }

    // Clear validation state when input changes
    connection_validated = false;

    // Disable Next button since input changed
    ui_wizard_set_next_button_enabled(false);
}

// ============================================================================
// Callback Registration
// ============================================================================

void ui_wizard_connection_register_callbacks() {
    spdlog::debug("[Wizard Connection] Registering event callbacks");

    // Register callbacks with lv_xml system
    lv_xml_register_event_cb(nullptr, "on_test_connection_clicked", on_test_connection_clicked);
    lv_xml_register_event_cb(nullptr, "on_ip_input_changed", on_ip_input_changed);
    lv_xml_register_event_cb(nullptr, "on_port_input_changed", on_port_input_changed);

    spdlog::info("[Wizard Connection] Event callbacks registered");
}

// ============================================================================
// Screen Creation
// ============================================================================

lv_obj_t* ui_wizard_connection_create(lv_obj_t* parent) {
    spdlog::debug("[Wizard Connection] Creating connection screen");

    if (!parent) {
        spdlog::error("[Wizard Connection] Cannot create: null parent");
        return nullptr;
    }

    // Create from XML
    connection_screen_root = (lv_obj_t*)lv_xml_create(parent, "wizard_connection", nullptr);

    if (!connection_screen_root) {
        spdlog::error("[Wizard Connection] Failed to create from XML");
        return nullptr;
    }

    // Find and configure test button (in case XML doesn't have event_cb)
    lv_obj_t* test_btn = lv_obj_find_by_name(connection_screen_root, "btn_test_connection");
    if (test_btn) {
        lv_obj_add_event_cb(test_btn, on_test_connection_clicked, LV_EVENT_CLICKED, nullptr);
        spdlog::debug("[Wizard Connection] Test button callback attached");
    } else {
        spdlog::warn("[Wizard Connection] Test button not found in XML");
    }

    // Find input fields and attach change handlers
    lv_obj_t* ip_input = lv_obj_find_by_name(connection_screen_root, "ip_input");
    if (ip_input) {
        lv_obj_add_event_cb(ip_input, on_ip_input_changed, LV_EVENT_VALUE_CHANGED, nullptr);
        spdlog::debug("[Wizard Connection] IP input change handler attached");
    }

    lv_obj_t* port_input = lv_obj_find_by_name(connection_screen_root, "port_input");
    if (port_input) {
        lv_obj_add_event_cb(port_input, on_port_input_changed, LV_EVENT_VALUE_CHANGED, nullptr);
        spdlog::debug("[Wizard Connection] Port input change handler attached");
    }

    // Update layout
    lv_obj_update_layout(connection_screen_root);

    spdlog::info("[Wizard Connection] Screen created successfully");
    return connection_screen_root;
}

// ============================================================================
// Cleanup
// ============================================================================

void ui_wizard_connection_cleanup() {
    spdlog::debug("[Wizard Connection] Cleaning up connection screen");

    // If a connection test is in progress, cancel it
    if (lv_subject_get_int(&connection_testing) == 1) {
        MoonrakerClient* client = get_moonraker_client();
        if (client) {
            client->close();
        }
        lv_subject_set_int(&connection_testing, 0);
    }

    // Clear status
    lv_subject_copy_string(&connection_status, "");

    // Reset UI references
    connection_screen_root = nullptr;

    spdlog::info("[Wizard Connection] Cleanup complete");
}

// ============================================================================
// Utility Functions
// ============================================================================

bool ui_wizard_connection_get_url(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        return false;
    }

    const char* ip = lv_subject_get_string(&connection_ip);
    const char* port_str = lv_subject_get_string(&connection_port);

    // Validate inputs
    if (!is_valid_ip_or_hostname(ip) || !is_valid_port(port_str)) {
        return false;
    }

    // Build URL
    snprintf(buffer, size, "ws://%s:%s/websocket", ip, port_str);
    return true;
}

bool ui_wizard_connection_is_validated() {
    return connection_validated;
}