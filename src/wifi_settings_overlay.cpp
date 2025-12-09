// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 HelixScreen Contributors

#include "wifi_settings_overlay.h"

#include "ui_nav.h"
#include "ui_subject_registry.h"

#include "network_tester.h"
#include "wifi_manager.h"
#include "wifi_ui_utils.h"

#include <lvgl/lvgl.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>

// ============================================================================
// Global Instance
// ============================================================================

static std::unique_ptr<WiFiSettingsOverlay> g_wifi_settings_overlay;

WiFiSettingsOverlay& get_wifi_settings_overlay() {
    if (!g_wifi_settings_overlay) {
        g_wifi_settings_overlay = std::make_unique<WiFiSettingsOverlay>();
    }
    return *g_wifi_settings_overlay;
}

void destroy_wifi_settings_overlay() {
    g_wifi_settings_overlay.reset();
}

// ============================================================================
// Helper Types
// ============================================================================

/**
 * @brief Per-instance network item data for click handling
 */
struct NetworkItemData {
    std::string ssid;
    bool is_secured;
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

WiFiSettingsOverlay::WiFiSettingsOverlay() {
    std::memset(ssid_buffer_, 0, sizeof(ssid_buffer_));
    std::memset(ip_buffer_, 0, sizeof(ip_buffer_));
    std::memset(mac_buffer_, 0, sizeof(mac_buffer_));
    std::memset(count_buffer_, 0, sizeof(count_buffer_));
    std::memset(current_ssid_, 0, sizeof(current_ssid_));

    spdlog::debug("[WiFiSettingsOverlay] Instance created");
}

WiFiSettingsOverlay::~WiFiSettingsOverlay() {
    // Clean up managers FIRST - they have background threads
    wifi_manager_.reset();
    network_tester_.reset();

    // Deinitialize subjects to disconnect observers
    if (subjects_initialized_) {
        lv_subject_deinit(&wifi_enabled_);
        lv_subject_deinit(&wifi_connected_);
        lv_subject_deinit(&connected_ssid_);
        lv_subject_deinit(&ip_address_);
        lv_subject_deinit(&mac_address_);
        lv_subject_deinit(&network_count_);
        lv_subject_deinit(&wifi_scanning_);
        lv_subject_deinit(&test_running_);
        lv_subject_deinit(&test_gateway_status_);
        lv_subject_deinit(&test_internet_status_);
        subjects_initialized_ = false;
    }

    // Clear widget pointers (owned by LVGL)
    overlay_root_ = nullptr;
    parent_screen_ = nullptr;
    networks_list_ = nullptr;
    // NOTE: Do NOT use spdlog here - it may be destroyed during exit
}

// ============================================================================
// Subject Initialization
// ============================================================================

void WiFiSettingsOverlay::init_subjects() {
    if (subjects_initialized_) {
        spdlog::debug("[WiFiSettingsOverlay] Subjects already initialized");
        return;
    }

    spdlog::debug("[WiFiSettingsOverlay] Initializing subjects");

    // Integer subjects
    UI_SUBJECT_INIT_AND_REGISTER_INT(wifi_enabled_, 0, "wifi_enabled");
    UI_SUBJECT_INIT_AND_REGISTER_INT(wifi_connected_, 0, "wifi_connected");
    UI_SUBJECT_INIT_AND_REGISTER_INT(wifi_scanning_, 0, "wifi_scanning");
    UI_SUBJECT_INIT_AND_REGISTER_INT(test_running_, 0, "test_running");
    UI_SUBJECT_INIT_AND_REGISTER_INT(test_gateway_status_, 0, "test_gateway_status");
    UI_SUBJECT_INIT_AND_REGISTER_INT(test_internet_status_, 0, "test_internet_status");

    // String subjects with buffers
    UI_SUBJECT_INIT_AND_REGISTER_STRING(connected_ssid_, ssid_buffer_, "", "connected_ssid");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(ip_address_, ip_buffer_, "", "ip_address");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(mac_address_, mac_buffer_, "", "mac_address");
    UI_SUBJECT_INIT_AND_REGISTER_STRING(network_count_, count_buffer_, "(0)", "network_count");

    subjects_initialized_ = true;
    spdlog::debug("[WiFiSettingsOverlay] Subjects initialized");
}

// ============================================================================
// Callback Registration
// ============================================================================

void WiFiSettingsOverlay::register_callbacks() {
    if (callbacks_registered_) {
        spdlog::debug("[WiFiSettingsOverlay] Callbacks already registered");
        return;
    }

    spdlog::debug("[WiFiSettingsOverlay] Registering event callbacks");

    lv_xml_register_event_cb(nullptr, "on_wlan_toggle_changed", on_wlan_toggle_changed);
    lv_xml_register_event_cb(nullptr, "on_refresh_clicked", on_refresh_clicked);
    lv_xml_register_event_cb(nullptr, "on_test_network_clicked", on_test_network_clicked);
    lv_xml_register_event_cb(nullptr, "on_add_other_clicked", on_add_other_clicked);
    lv_xml_register_event_cb(nullptr, "on_network_item_clicked", on_network_item_clicked);

    callbacks_registered_ = true;
    spdlog::debug("[WiFiSettingsOverlay] Event callbacks registered");
}

// ============================================================================
// Screen Creation
// ============================================================================

lv_obj_t* WiFiSettingsOverlay::create(lv_obj_t* parent_screen) {
    if (!parent_screen) {
        spdlog::error("[WiFiSettingsOverlay] Cannot create: null parent_screen");
        return nullptr;
    }

    spdlog::debug("[WiFiSettingsOverlay] Creating overlay from XML");

    parent_screen_ = parent_screen;

    // Reset cleanup flag when (re)creating
    cleanup_called_ = false;

    // Register wifi_network_item component first
    static bool network_item_registered = false;
    if (!network_item_registered) {
        lv_xml_register_component_from_file("A:ui_xml/wifi_network_item.xml");
        network_item_registered = true;
        spdlog::debug("[WiFiSettingsOverlay] Registered wifi_network_item component");
    }

    // Create overlay from XML
    overlay_root_ =
        static_cast<lv_obj_t*>(lv_xml_create(parent_screen, "wifi_settings_overlay", nullptr));

    if (!overlay_root_) {
        spdlog::error("[WiFiSettingsOverlay] Failed to create from XML");
        return nullptr;
    }

    // Get reference to networks_list for population
    networks_list_ = lv_obj_find_by_name(overlay_root_, "networks_list");
    if (!networks_list_) {
        spdlog::error("[WiFiSettingsOverlay] networks_list not found in XML");
        return nullptr;
    }

    // Wire up header bar back button
    lv_obj_t* header = lv_obj_find_by_name(overlay_root_, "overlay_header");
    if (header) {
        lv_obj_t* back_btn = lv_obj_find_by_name(header, "back_button");
        if (back_btn) {
            lv_obj_add_event_cb(
                back_btn,
                [](lv_event_t*) {
                    spdlog::debug("[WiFiSettingsOverlay] Back button clicked");
                    ui_nav_go_back();
                },
                LV_EVENT_CLICKED, nullptr);
            spdlog::debug("[WiFiSettingsOverlay] Back button wired");
        }
    }

    // Initially hidden
    lv_obj_add_flag(overlay_root_, LV_OBJ_FLAG_HIDDEN);

    // Initialize WiFi manager
    if (!wifi_manager_) {
        wifi_manager_ = std::make_shared<WiFiManager>();
        wifi_manager_->init_self_reference(wifi_manager_);
        spdlog::debug("[WiFiSettingsOverlay] WiFiManager initialized");
    }

    // Initialize NetworkTester
    if (!network_tester_) {
        network_tester_ = std::make_shared<NetworkTester>();
        network_tester_->init_self_reference(network_tester_);
        spdlog::debug("[WiFiSettingsOverlay] NetworkTester initialized");
    }

    // Update initial connection status
    update_connection_status();

    spdlog::info("[WiFiSettingsOverlay] Overlay created successfully");
    return overlay_root_;
}

// ============================================================================
// Show/Hide
// ============================================================================

void WiFiSettingsOverlay::show() {
    if (!overlay_root_) {
        spdlog::error("[WiFiSettingsOverlay] Cannot show: overlay not created");
        return;
    }

    spdlog::debug("[WiFiSettingsOverlay] Showing overlay");

    visible_ = true;
    ui_nav_push_overlay(overlay_root_);

    // Update connection status
    update_connection_status();

    // Start scanning if WiFi enabled
    if (wifi_manager_ && wifi_manager_->is_enabled()) {
        lv_subject_set_int(&wifi_scanning_, 1);

        std::weak_ptr<WiFiManager> weak_mgr = wifi_manager_;
        WiFiSettingsOverlay* self = this;

        wifi_manager_->start_scan([self, weak_mgr](const std::vector<WiFiNetwork>& networks) {
            // Check if manager still exists
            if (weak_mgr.expired()) {
                spdlog::debug("[WiFiSettingsOverlay] WiFiManager destroyed, ignoring callback");
                return;
            }

            // Check if cleanup was called
            if (self->cleanup_called_) {
                spdlog::debug("[WiFiSettingsOverlay] Cleanup called, ignoring stale scan callback");
                return;
            }

            lv_subject_set_int(&self->wifi_scanning_, 0);
            self->populate_network_list(networks);
        });
    }

    spdlog::info("[WiFiSettingsOverlay] Overlay shown");
}

void WiFiSettingsOverlay::hide() {
    if (!overlay_root_) {
        return;
    }

    spdlog::debug("[WiFiSettingsOverlay] Hiding overlay");

    visible_ = false;

    // Stop scanning
    if (wifi_manager_) {
        wifi_manager_->stop_scan();
        lv_subject_set_int(&wifi_scanning_, 0);
    }

    // Cancel any running tests
    if (network_tester_ && network_tester_->is_running()) {
        network_tester_->cancel();
        lv_subject_set_int(&test_running_, 0);
    }

    lv_obj_add_flag(overlay_root_, LV_OBJ_FLAG_HIDDEN);

    spdlog::info("[WiFiSettingsOverlay] Overlay hidden");
}

// ============================================================================
// Cleanup
// ============================================================================

void WiFiSettingsOverlay::cleanup() {
    spdlog::debug("[WiFiSettingsOverlay] Cleaning up");

    // Mark as cleaned up FIRST to invalidate any pending async callbacks
    cleanup_called_ = true;

    if (wifi_manager_) {
        wifi_manager_->stop_scan();
    }

    if (network_tester_ && network_tester_->is_running()) {
        network_tester_->cancel();
    }

    clear_network_list();

    wifi_manager_.reset();
    network_tester_.reset();

    overlay_root_ = nullptr;
    parent_screen_ = nullptr;
    networks_list_ = nullptr;
    visible_ = false;

    current_ssid_[0] = '\0';
    current_network_is_secured_ = false;

    spdlog::debug("[WiFiSettingsOverlay] Cleanup complete");
}

// ============================================================================
// Helper Functions
// ============================================================================

void WiFiSettingsOverlay::update_connection_status() {
    if (!wifi_manager_) {
        spdlog::debug("[WiFiSettingsOverlay] Cannot update connection status: no WiFiManager");
        return;
    }

    bool enabled = wifi_manager_->is_enabled();
    bool connected = wifi_manager_->is_connected();

    lv_subject_set_int(&wifi_enabled_, enabled ? 1 : 0);
    lv_subject_set_int(&wifi_connected_, connected ? 1 : 0);

    if (connected) {
        std::string ssid = wifi_manager_->get_connected_ssid();
        std::string ip = wifi_manager_->get_ip_address();
        std::string mac = wifi_ui::wifi_get_device_mac();

        strncpy(ssid_buffer_, ssid.c_str(), sizeof(ssid_buffer_) - 1);
        ssid_buffer_[sizeof(ssid_buffer_) - 1] = '\0';
        lv_subject_notify(&connected_ssid_);

        strncpy(ip_buffer_, ip.c_str(), sizeof(ip_buffer_) - 1);
        ip_buffer_[sizeof(ip_buffer_) - 1] = '\0';
        lv_subject_notify(&ip_address_);

        strncpy(mac_buffer_, mac.c_str(), sizeof(mac_buffer_) - 1);
        mac_buffer_[sizeof(mac_buffer_) - 1] = '\0';
        lv_subject_notify(&mac_address_);

        spdlog::debug("[WiFiSettingsOverlay] Connected: {} ({})", ssid, ip);
    } else {
        ssid_buffer_[0] = '\0';
        ip_buffer_[0] = '\0';
        mac_buffer_[0] = '\0';
        lv_subject_notify(&connected_ssid_);
        lv_subject_notify(&ip_address_);
        lv_subject_notify(&mac_address_);
    }
}

void WiFiSettingsOverlay::update_test_state(NetworkTester::TestState state,
                                            const NetworkTester::TestResult& result) {
    spdlog::debug("[WiFiSettingsOverlay] Test state: {}", static_cast<int>(state));

    switch (state) {
    case NetworkTester::TestState::IDLE:
        lv_subject_set_int(&test_running_, 0);
        lv_subject_set_int(&test_gateway_status_, 0);
        lv_subject_set_int(&test_internet_status_, 0);
        break;

    case NetworkTester::TestState::TESTING_GATEWAY:
        lv_subject_set_int(&test_running_, 1);
        lv_subject_set_int(&test_gateway_status_, 1);  // active
        lv_subject_set_int(&test_internet_status_, 0); // pending
        break;

    case NetworkTester::TestState::TESTING_INTERNET:
        lv_subject_set_int(&test_running_, 1);
        lv_subject_set_int(&test_gateway_status_, result.gateway_ok ? 2 : 3); // success/failed
        lv_subject_set_int(&test_internet_status_, 1);                        // active
        break;

    case NetworkTester::TestState::COMPLETED:
        lv_subject_set_int(&test_running_, 0);
        lv_subject_set_int(&test_gateway_status_, result.gateway_ok ? 2 : 3);
        lv_subject_set_int(&test_internet_status_, result.internet_ok ? 2 : 3);
        spdlog::info("[WiFiSettingsOverlay] Test complete - Gateway: {}, Internet: {}",
                     result.gateway_ok ? "OK" : "FAIL", result.internet_ok ? "OK" : "FAIL");
        break;

    case NetworkTester::TestState::FAILED:
        lv_subject_set_int(&test_running_, 0);
        lv_subject_set_int(&test_gateway_status_, 3);  // failed
        lv_subject_set_int(&test_internet_status_, 3); // failed
        spdlog::warn("[WiFiSettingsOverlay] Test failed: {}", result.error_message);
        break;
    }
}

void WiFiSettingsOverlay::populate_network_list(const std::vector<WiFiNetwork>& networks) {
    if (!networks_list_) {
        spdlog::error("[WiFiSettingsOverlay] Cannot populate: networks_list is null");
        return;
    }

    spdlog::debug("[WiFiSettingsOverlay] Populating network list with {} networks",
                  networks.size());

    // Save scroll position before clearing
    int32_t scroll_y = lv_obj_get_scroll_y(networks_list_);

    clear_network_list();

    // Update count
    snprintf(count_buffer_, sizeof(count_buffer_), "(%zu)", networks.size());
    lv_subject_notify(&network_count_);

    // Show/hide placeholder
    show_placeholder(networks.empty());

    // Sort by signal strength
    std::vector<WiFiNetwork> sorted_networks = networks;
    std::sort(sorted_networks.begin(), sorted_networks.end(),
              [](const WiFiNetwork& a, const WiFiNetwork& b) {
                  return a.signal_strength > b.signal_strength;
              });

    // Get connected network SSID
    std::string connected_ssid;
    if (wifi_manager_) {
        connected_ssid = wifi_manager_->get_connected_ssid();
    }

    // Create network items
    static int item_counter = 0;
    for (const auto& network : sorted_networks) {
        lv_obj_t* item =
            static_cast<lv_obj_t*>(lv_xml_create(networks_list_, "wifi_network_item", nullptr));
        if (!item) {
            spdlog::error("[WiFiSettingsOverlay] Failed to create network item for SSID: {}",
                          network.ssid);
            continue;
        }

        char item_name[32];
        snprintf(item_name, sizeof(item_name), "network_item_%d", item_counter++);
        lv_obj_set_name(item, item_name);

        // Set SSID
        lv_obj_t* ssid_label = lv_obj_find_by_name(item, "ssid_label");
        if (ssid_label) {
            lv_label_set_text(ssid_label, network.ssid.c_str());
        }

        // Set security label
        lv_obj_t* security_label = lv_obj_find_by_name(item, "security_label");
        if (security_label) {
            if (network.is_secured) {
                lv_label_set_text(security_label, network.security_type.c_str());
            } else {
                lv_label_set_text(security_label, "");
            }
        }

        // Update signal icons
        int icon_state =
            wifi_ui::wifi_compute_signal_icon_state(network.signal_strength, network.is_secured);
        update_signal_icons(item, icon_state);

        // Mark connected network with LV_STATE_CHECKED
        bool is_connected = (!connected_ssid.empty() && network.ssid == connected_ssid);
        if (is_connected) {
            lv_obj_add_state(item, LV_STATE_CHECKED);
            spdlog::debug("[WiFiSettingsOverlay] Marked connected network: {}", network.ssid);
        }

        // Store network data for click handler
        auto* data = new NetworkItemData{network.ssid, network.is_secured};
        lv_obj_set_user_data(item, data);

        spdlog::debug("[WiFiSettingsOverlay] Added network: {} ({}%, {})", network.ssid,
                      network.signal_strength, network.is_secured ? "secured" : "open");
    }

    // Restore scroll position
    lv_obj_update_layout(networks_list_);
    lv_obj_scroll_to_y(networks_list_, scroll_y, LV_ANIM_OFF);

    spdlog::debug("[WiFiSettingsOverlay] Populated {} network items", sorted_networks.size());
}

void WiFiSettingsOverlay::clear_network_list() {
    if (!networks_list_) {
        return;
    }

    spdlog::debug("[WiFiSettingsOverlay] Clearing network list");

    int32_t child_count = static_cast<int32_t>(lv_obj_get_child_count(networks_list_));

    // Iterate in reverse to avoid index shifting
    for (int32_t i = child_count - 1; i >= 0; i--) {
        lv_obj_t* child = lv_obj_get_child(networks_list_, i);
        if (!child)
            continue;

        const char* name = lv_obj_get_name(child);
        if (name && strncmp(name, "network_item_", 13) == 0) {
            // Delete user data
            NetworkItemData* item_data = static_cast<NetworkItemData*>(lv_obj_get_user_data(child));
            if (item_data) {
                delete item_data;
            }

            lv_obj_delete(child);
        }
    }

    spdlog::debug("[WiFiSettingsOverlay] Network list cleared");
}

void WiFiSettingsOverlay::show_placeholder(bool show) {
    if (!networks_list_) {
        return;
    }

    lv_obj_t* placeholder = lv_obj_find_by_name(networks_list_, "no_networks_placeholder");
    if (placeholder) {
        if (show) {
            lv_obj_remove_flag(placeholder, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(placeholder, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void WiFiSettingsOverlay::update_signal_icons(lv_obj_t* item, int icon_state) {
    if (!item) {
        return;
    }

    lv_obj_t* signal_icons = lv_obj_find_by_name(item, "signal_icons");
    if (!signal_icons) {
        return;
    }

    // Icon names and their corresponding states
    static const struct {
        const char* name;
        int state;
    } icon_bindings[] = {
        {"sig_1", 1},      {"sig_2", 2},      {"sig_3", 3},      {"sig_4", 4},
        {"sig_1_lock", 5}, {"sig_2_lock", 6}, {"sig_3_lock", 7}, {"sig_4_lock", 8},
    };

    // Show only the icon matching current state
    for (const auto& binding : icon_bindings) {
        lv_obj_t* icon = lv_obj_find_by_name(signal_icons, binding.name);
        if (icon) {
            if (binding.state == icon_state) {
                lv_obj_remove_flag(icon, LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

// ============================================================================
// Event Handler Implementations
// ============================================================================

void WiFiSettingsOverlay::handle_wlan_toggle_changed(lv_event_t* e) {
    lv_obj_t* sw = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (!sw)
        return;

    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    spdlog::info("[WiFiSettingsOverlay] WLAN toggle: {}", enabled ? "ON" : "OFF");

    if (!wifi_manager_) {
        spdlog::error("[WiFiSettingsOverlay] WiFiManager not initialized");
        return;
    }

    wifi_manager_->set_enabled(enabled);
    lv_subject_set_int(&wifi_enabled_, enabled ? 1 : 0);

    if (enabled) {
        // Start scanning
        lv_subject_set_int(&wifi_scanning_, 1);

        std::weak_ptr<WiFiManager> weak_mgr = wifi_manager_;
        WiFiSettingsOverlay* self = this;

        wifi_manager_->start_scan([self, weak_mgr](const std::vector<WiFiNetwork>& networks) {
            if (weak_mgr.expired() || self->cleanup_called_) {
                return;
            }

            lv_subject_set_int(&self->wifi_scanning_, 0);
            self->populate_network_list(networks);
        });
    } else {
        // Stop scanning, clear list
        wifi_manager_->stop_scan();
        lv_subject_set_int(&wifi_scanning_, 0);
        clear_network_list();
        show_placeholder(true);

        // Update connection status
        lv_subject_set_int(&wifi_connected_, 0);
        ssid_buffer_[0] = '\0';
        ip_buffer_[0] = '\0';
        mac_buffer_[0] = '\0';
        lv_subject_notify(&connected_ssid_);
        lv_subject_notify(&ip_address_);
        lv_subject_notify(&mac_address_);
    }
}

void WiFiSettingsOverlay::handle_refresh_clicked() {
    spdlog::debug("[WiFiSettingsOverlay] Refresh clicked");

    if (!wifi_manager_ || !wifi_manager_->is_enabled()) {
        spdlog::warn("[WiFiSettingsOverlay] Cannot refresh: WiFi not enabled");
        return;
    }

    lv_subject_set_int(&wifi_scanning_, 1);

    std::weak_ptr<WiFiManager> weak_mgr = wifi_manager_;
    WiFiSettingsOverlay* self = this;

    wifi_manager_->start_scan([self, weak_mgr](const std::vector<WiFiNetwork>& networks) {
        if (weak_mgr.expired() || self->cleanup_called_) {
            return;
        }

        lv_subject_set_int(&self->wifi_scanning_, 0);
        self->populate_network_list(networks);
    });
}

void WiFiSettingsOverlay::handle_test_network_clicked() {
    spdlog::debug("[WiFiSettingsOverlay] Test network clicked");

    if (!network_tester_) {
        spdlog::error("[WiFiSettingsOverlay] NetworkTester not initialized");
        return;
    }

    if (!wifi_manager_ || !wifi_manager_->is_connected()) {
        spdlog::warn("[WiFiSettingsOverlay] Cannot test: not connected");
        return;
    }

    // Reset test status
    lv_subject_set_int(&test_gateway_status_, 0);
    lv_subject_set_int(&test_internet_status_, 0);
    lv_subject_set_int(&test_running_, 1);

    WiFiSettingsOverlay* self = this;

    network_tester_->start_test(
        [self](NetworkTester::TestState state, const NetworkTester::TestResult& result) {
            // Use lv_async_call for thread safety
            struct CallbackData {
                WiFiSettingsOverlay* overlay;
                NetworkTester::TestState state;
                NetworkTester::TestResult result;
            };

            auto* data = new CallbackData{self, state, result};

            lv_async_call(
                [](void* ctx) {
                    auto* cb_data = static_cast<CallbackData*>(ctx);
                    if (!cb_data->overlay->cleanup_called_) {
                        cb_data->overlay->update_test_state(cb_data->state, cb_data->result);
                    }
                    delete cb_data;
                },
                data);
        });
}

void WiFiSettingsOverlay::handle_add_other_clicked() {
    spdlog::debug("[WiFiSettingsOverlay] Add other networks clicked");
    // TODO: Show hidden network modal
    spdlog::warn("[WiFiSettingsOverlay] Hidden network modal not yet implemented");
}

void WiFiSettingsOverlay::handle_network_item_clicked(lv_event_t* e) {
    lv_obj_t* item = static_cast<lv_obj_t*>(lv_event_get_target(e));
    if (!item)
        return;

    NetworkItemData* item_data = static_cast<NetworkItemData*>(lv_obj_get_user_data(item));
    if (!item_data) {
        spdlog::error("[WiFiSettingsOverlay] No network data found in clicked item");
        return;
    }

    spdlog::info("[WiFiSettingsOverlay] Network clicked: {} ({})", item_data->ssid,
                 item_data->is_secured ? "secured" : "open");

    strncpy(current_ssid_, item_data->ssid.c_str(), sizeof(current_ssid_) - 1);
    current_ssid_[sizeof(current_ssid_) - 1] = '\0';
    current_network_is_secured_ = item_data->is_secured;

    if (item_data->is_secured) {
        // TODO: Show password modal
        spdlog::warn("[WiFiSettingsOverlay] Password modal not yet implemented");
    } else {
        // Connect to open network
        if (!wifi_manager_) {
            spdlog::error("[WiFiSettingsOverlay] WiFiManager not initialized");
            return;
        }

        WiFiSettingsOverlay* self = this;
        wifi_manager_->connect(item_data->ssid, "", [self](bool success, const std::string& error) {
            if (self->cleanup_called_) {
                return;
            }

            if (success) {
                spdlog::info("[WiFiSettingsOverlay] Connected to {}", self->current_ssid_);
                self->update_connection_status();
            } else {
                spdlog::error("[WiFiSettingsOverlay] Failed to connect: {}", error);
            }
        });
    }
}

// ============================================================================
// Static Trampolines for LVGL Callbacks
// ============================================================================

void WiFiSettingsOverlay::on_wlan_toggle_changed(lv_event_t* e) {
    auto& self = get_wifi_settings_overlay();
    self.handle_wlan_toggle_changed(e);
}

void WiFiSettingsOverlay::on_refresh_clicked(lv_event_t* e) {
    auto& self = get_wifi_settings_overlay();
    self.handle_refresh_clicked();
}

void WiFiSettingsOverlay::on_test_network_clicked(lv_event_t* e) {
    auto& self = get_wifi_settings_overlay();
    self.handle_test_network_clicked();
}

void WiFiSettingsOverlay::on_add_other_clicked(lv_event_t* e) {
    auto& self = get_wifi_settings_overlay();
    self.handle_add_other_clicked();
}

void WiFiSettingsOverlay::on_network_item_clicked(lv_event_t* e) {
    auto& self = get_wifi_settings_overlay();
    self.handle_network_item_clicked(e);
}
