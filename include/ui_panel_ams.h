// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "ui_observer_guard.h"
#include "ui_panel_base.h"

#include "ams_state.h"

/**
 * @file ui_panel_ams.h
 * @brief AMS/Multi-filament panel - slot visualization and operations
 *
 * Displays a Bambu-inspired visualization of multi-filament units (Happy Hare, AFC)
 * with colored slots, status indicators, and load/unload operations.
 *
 * ## UI Layout (480x800 primary target):
 * ```
 * ┌─────────────────────────────────────────┐
 * │ header_bar: "Multi-Filament"            │
 * ├─────────────────────────────────────────┤
 * │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐   │
 * │  │ Slot │ │ Slot │ │ Slot │ │ Slot │   │
 * │  │  0   │ │  1   │ │  2   │ │  3   │   │
 * │  └──────┘ └──────┘ └──────┘ └──────┘   │
 * │                                         │
 * │  [Status: Idle / Loading / etc.]        │
 * │                                         │
 * │  [Action buttons: Unload, Home, etc.]   │
 * └─────────────────────────────────────────┘
 * ```
 *
 * ## Reactive Bindings:
 * - Gate colors: `ams_gate_N_color` (int, RGB packed)
 * - Gate status: `ams_gate_N_status` (int, GateStatus enum)
 * - Current gate: `ams_current_gate` (int, -1 if none)
 * - Action: `ams_action` (int, AmsAction enum)
 * - Action detail: `ams_action_detail` (string)
 *
 * @see AmsState for subject definitions
 * @see AmsBackend for backend operations
 */
class AmsPanel : public PanelBase {
  public:
    /**
     * @brief Construct AMS panel with dependencies
     * @param printer_state Reference to global PrinterState
     * @param api Pointer to MoonrakerAPI (may be nullptr)
     */
    AmsPanel(PrinterState& printer_state, MoonrakerAPI* api);
    ~AmsPanel() override = default;

    // === PanelBase Interface ===

    void init_subjects() override;
    void setup(lv_obj_t* panel, lv_obj_t* parent_screen) override;
    void on_activate() override;
    void on_deactivate() override;

    [[nodiscard]] const char* get_name() const override {
        return "AMS Panel";
    }

    [[nodiscard]] const char* get_xml_component_name() const override {
        return "ams_panel";
    }

    // === Public API ===

    /**
     * @brief Get the root panel object
     * @return Panel widget, or nullptr if not setup
     */
    [[nodiscard]] lv_obj_t* get_panel() const {
        return panel_;
    }

    /**
     * @brief Refresh slot display from backend state
     *
     * Call this after backend operations complete to update UI.
     * Normally handled automatically via AmsState observer callbacks.
     */
    void refresh_slots();

  private:
    // === Slot Management ===

    static constexpr int MAX_VISIBLE_SLOTS = 8; ///< Max slots displayed (2 rows of 4)
    lv_obj_t* slot_widgets_[MAX_VISIBLE_SLOTS] = {nullptr};

    // === Observers (RAII cleanup via ObserverGuard) ===

    ObserverGuard gates_version_observer_;
    ObserverGuard action_observer_;
    ObserverGuard current_gate_observer_;

    // === Setup Helpers ===

    void setup_slots();
    void setup_action_buttons();
    void setup_status_display();

    // === UI Update Handlers ===

    void update_slot_colors();
    void update_slot_status(int gate_index);
    void update_action_display(AmsAction action);
    void update_current_gate_highlight(int gate_index);

    // === Event Callbacks (static trampolines) ===

    static void on_slot_clicked(lv_event_t* e);
    static void on_unload_clicked(lv_event_t* e);
    static void on_home_clicked(lv_event_t* e);

    // === Observer Callbacks ===

    static void on_gates_version_changed(lv_observer_t* observer, lv_subject_t* subject);
    static void on_action_changed(lv_observer_t* observer, lv_subject_t* subject);
    static void on_current_gate_changed(lv_observer_t* observer, lv_subject_t* subject);

    // === Action Handlers ===

    void handle_slot_tap(int slot_index);
    void handle_unload();
    void handle_home();
};

/**
 * @brief Get global AMS panel singleton
 *
 * Creates the panel on first call, returns cached instance thereafter.
 * Panel is lazily initialized - subjects created but XML not until setup.
 *
 * @return Reference to global AmsPanel instance
 */
AmsPanel& get_global_ams_panel();
