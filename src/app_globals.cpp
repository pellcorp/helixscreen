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

/**
 * @file app_globals.cpp
 * @brief Global application state and accessors
 *
 * Provides centralized access to global singleton instances like MoonrakerClient,
 * PrinterState, and reactive subjects. This module exists to:
 * 1. Keep main.cpp cleaner and more focused
 * 2. Provide a single point of truth for global state
 * 3. Make it easier to add new global subjects/singletons
 */

#include "app_globals.h"
#include "moonraker_client.h"
#include "moonraker_api.h"
#include "printer_state.h"
#include <spdlog/spdlog.h>

// Global singleton instances (extern declarations in header, definitions here)
// These are set by main.cpp during initialization
static MoonrakerClient* g_moonraker_client = nullptr;
static MoonrakerAPI* g_moonraker_api = nullptr;

// Global reactive subjects
static lv_subject_t g_notification_subject;

MoonrakerClient* get_moonraker_client() {
    return g_moonraker_client;
}

void set_moonraker_client(MoonrakerClient* client) {
    g_moonraker_client = client;
}

MoonrakerAPI* get_moonraker_api() {
    return g_moonraker_api;
}

void set_moonraker_api(MoonrakerAPI* api) {
    g_moonraker_api = api;
}

PrinterState& get_printer_state() {
    // Singleton instance - created once, lives for lifetime of program
    static PrinterState instance;
    return instance;
}

lv_subject_t& get_notification_subject() {
    return g_notification_subject;
}

void app_globals_init_subjects() {
    // Initialize notification subject (stores NotificationData pointer)
    lv_subject_init_pointer(&g_notification_subject, nullptr);

    spdlog::debug("Global subjects initialized");
}
