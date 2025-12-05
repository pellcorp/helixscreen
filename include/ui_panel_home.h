// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_observer_guard.h"
#include "ui_panel_base.h"

#include "tips_manager.h"

#include <memory>

// Forward declaration
class WiFiManager;

/**
 * @brief Home panel - Main dashboard showing printer status and quick actions
 *
 * Displays printer image, temperature, network status, light toggle, and
 * tip of the day with auto-rotation. Responsive sizing based on screen dimensions.
 *
 * @see TipsManager for tip of the day functionality
 */

// Network connection types
typedef enum { NETWORK_WIFI, NETWORK_ETHERNET, NETWORK_DISCONNECTED } network_type_t;

class HomePanel : public PanelBase {
  public:
    /**
     * @brief Construct HomePanel with injected dependencies
     * @param printer_state Reference to PrinterState
     * @param api Pointer to MoonrakerAPI (for light control)
     */
    HomePanel(PrinterState& printer_state, MoonrakerAPI* api);
    ~HomePanel() override;

    void init_subjects() override;
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen) override;
    void on_activate() override;
    void on_deactivate() override;
    const char* get_name() const override {
        return "Home Panel";
    }
    const char* get_xml_component_name() const override {
        return "home_panel";
    }

    /**
     * @brief Update status text and temperature display
     * @param status_text New status/tip text (nullptr to keep current)
     * @param temp Temperature in degrees Celsius
     */
    void update(const char* status_text, int temp);

    /** @brief Set network status display */
    void set_network(network_type_t type);

    /** @brief Set light state (on=gold, off=grey) */
    void set_light(bool is_on);

    bool get_light_state() const {
        return light_on_;
    }

  private:
    lv_subject_t status_subject_;
    lv_subject_t temp_subject_;
    lv_subject_t network_icon_state_; // Integer subject: 0-5 for conditional icon visibility
    lv_subject_t network_label_subject_;

    // Legacy string subjects (kept for network_label binding)
    lv_subject_t network_icon_subject_;  // Unused after migration
    lv_subject_t network_color_subject_; // Unused after migration

    char status_buffer_[512];
    char temp_buffer_[32];
    char network_icon_buffer_[8];
    char network_label_buffer_[32];
    char network_color_buffer_[16];

    bool light_on_ = false;
    network_type_t current_network_ = NETWORK_WIFI;
    PrintingTip current_tip_;
    std::string configured_led_;
    lv_timer_t* tip_rotation_timer_ = nullptr;
    lv_timer_t* signal_poll_timer_ = nullptr; // Polls WiFi signal strength every 5s
    lv_obj_t* light_button_ = nullptr;
    lv_obj_t* light_divider_ = nullptr;
    lv_obj_t* printer_image_ = nullptr;

    std::shared_ptr<WiFiManager> wifi_manager_; // For signal strength queries

    void update_tip_of_day();
    int compute_network_icon_state(); // Maps network type + signal → 0-5
    void update_network_icon_state(); // Updates the subject
    static void signal_poll_timer_cb(lv_timer_t* timer);
    void setup_responsive_icon_fonts();
    void update_printer_image_opacity(int connection_state);

    void handle_light_toggle();
    void handle_print_card_clicked();
    void handle_tip_text_clicked();
    void handle_tip_rotation_timer();
    void on_extruder_temp_changed(int temp);
    void on_led_state_changed(int state);

    static void light_toggle_cb(lv_event_t* e);
    static void print_card_clicked_cb(lv_event_t* e);
    static void tip_text_clicked_cb(lv_event_t* e);
    static void tip_rotation_timer_cb(lv_timer_t* timer);
    static void extruder_temp_observer_cb(lv_observer_t* observer, lv_subject_t* subject);
    static void led_state_observer_cb(lv_observer_t* observer, lv_subject_t* subject);
    static void connection_state_observer_cb(lv_observer_t* observer, lv_subject_t* subject);

    ObserverGuard extruder_temp_observer_;
    ObserverGuard led_state_observer_;
    ObserverGuard connection_state_observer_;
};

// Global instance accessor (needed by main.cpp)
HomePanel& get_global_home_panel();
