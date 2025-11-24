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

#ifndef WIZARD_CONFIG_PATHS_H
#define WIZARD_CONFIG_PATHS_H

/**
 * @file wizard_config_paths.h
 * @brief Centralized configuration paths for wizard screens
 *
 * Defines all JSON configuration paths used by wizard screens to eliminate
 * hardcoded string literals and reduce typo risk.
 *
 * Paths now use structured format under /printers/default_printer/... to align
 * with the expected structure checked by Config::is_wizard_required().
 */

namespace WizardConfigPaths {
// Printer identification
constexpr const char* DEFAULT_PRINTER = "/default_printer";
constexpr const char* PRINTER_NAME = "/printers/default_printer/name";
constexpr const char* PRINTER_TYPE = "/printers/default_printer/type";

// Bed hardware
constexpr const char* BED_HEATER = "/printers/default_printer/heater/bed";
constexpr const char* BED_SENSOR = "/printers/default_printer/sensor/bed";

// Hotend hardware
constexpr const char* HOTEND_HEATER = "/printers/default_printer/heater/hotend";
constexpr const char* HOTEND_SENSOR = "/printers/default_printer/sensor/hotend";

// Fan hardware
constexpr const char* HOTEND_FAN = "/printers/default_printer/fan/hotend";
constexpr const char* PART_FAN = "/printers/default_printer/fan/part";

// LED hardware
constexpr const char* LED_STRIP = "/printers/default_printer/led/strip";

// Network configuration
// Note: Connection screen constructs full path dynamically using default_printer
// e.g., "/printers/" + default_printer + "/moonraker_host"
// These constants are for direct access in ui_wizard_complete()
constexpr const char* MOONRAKER_HOST = "/printers/default_printer/moonraker_host";
constexpr const char* MOONRAKER_PORT = "/printers/default_printer/moonraker_port";
constexpr const char* WIFI_SSID = "/wifi/ssid";
constexpr const char* WIFI_PASSWORD = "/wifi/password";
} // namespace WizardConfigPaths

#endif // WIZARD_CONFIG_PATHS_H
