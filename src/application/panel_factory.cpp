// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 HelixScreen

#include "panel_factory.h"

#include "ui_component_keypad.h"
#include "ui_nav.h"
#include "ui_panel_advanced.h"
#include "ui_panel_controls.h"
#include "ui_panel_filament.h"
#include "ui_panel_home.h"
#include "ui_panel_print_select.h"
#include "ui_panel_print_status.h"
#include "ui_panel_settings.h"

#include "app_globals.h"
#include "lvgl/lvgl.h"
#include "lvgl/src/xml/lv_xml.h"
#include "printer_state.h"

#include <spdlog/spdlog.h>

bool PanelFactory::find_panels(lv_obj_t* panel_container) {
    for (int i = 0; i < UI_PANEL_COUNT; i++) {
        m_panels[i] = lv_obj_find_by_name(panel_container, PANEL_NAMES[i]);
        if (!m_panels[i]) {
            spdlog::error("[PanelFactory] Missing panel '{}' in container", PANEL_NAMES[i]);
            return false;
        }
    }
    spdlog::debug("[PanelFactory] Found all {} panels", static_cast<int>(UI_PANEL_COUNT));
    return true;
}

void PanelFactory::setup_panels(lv_obj_t* screen) {
    // Register panels with navigation system
    ui_nav_set_panels(m_panels.data());

    // Setup home panel
    get_global_home_panel().setup(m_panels[UI_PANEL_HOME], screen);

    // Setup controls panel
    get_global_controls_panel().setup(m_panels[UI_PANEL_CONTROLS], screen);

    // Setup print select panel
    get_print_select_panel(get_printer_state(), nullptr)
        ->setup(m_panels[UI_PANEL_PRINT_SELECT], screen);

    // Setup filament panel
    get_global_filament_panel().setup(m_panels[UI_PANEL_FILAMENT], screen);

    // Setup settings panel
    get_global_settings_panel().setup(m_panels[UI_PANEL_SETTINGS], screen);

    // Setup advanced panel
    get_global_advanced_panel().setup(m_panels[UI_PANEL_ADVANCED], screen);

    spdlog::debug("[PanelFactory] All panels set up");
}

bool PanelFactory::create_print_status_overlay(lv_obj_t* screen) {
    m_print_status_panel = create_overlay(screen, "print_status_panel", "print status");
    if (!m_print_status_panel) {
        return false;
    }

    get_global_print_status_panel().setup(m_print_status_panel, screen);
    lv_obj_add_flag(m_print_status_panel, LV_OBJ_FLAG_HIDDEN);

    // Wire to print select panel
    get_print_select_panel(get_printer_state(), nullptr)
        ->set_print_status_panel(m_print_status_panel);

    spdlog::debug("[PanelFactory] Print status overlay created and wired");
    return true;
}

void PanelFactory::init_keypad(lv_obj_t* screen) {
    ui_keypad_init(screen);
}

lv_obj_t* PanelFactory::create_overlay(lv_obj_t* screen, const char* component_name,
                                       const char* display_name) {
    spdlog::debug("[PanelFactory] Creating {} overlay", display_name);
    lv_obj_t* panel = (lv_obj_t*)lv_xml_create(screen, component_name, nullptr);
    if (!panel) {
        spdlog::error("[PanelFactory] Failed to create {} overlay from '{}'", display_name,
                      component_name);
    }
    return panel;
}
