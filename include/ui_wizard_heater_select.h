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

#pragma once

#include "lvgl/lvgl.h"

/**
 * @brief Initialize subjects for heater select screen
 *
 * Creates and registers reactive subjects:
 * - bed_heater_selected (int) - Selected bed heater index in dropdown
 * - hotend_heater_selected (int) - Selected hotend heater index in dropdown
 *
 * Note: This screen does not use separate sensor dropdowns. The selected
 * heater names are automatically used as sensor names since Klipper heater
 * objects inherently provide temperature readings.
 */
void ui_wizard_heater_select_init_subjects();

/**
 * @brief Register event callbacks for heater select screen
 *
 * Registers callbacks for dropdown changes (attached programmatically)
 */
void ui_wizard_heater_select_register_callbacks();

/**
 * @brief Create heater select screen UI
 *
 * Creates the combined bed + hotend heater selection form from
 * wizard_heater_select.xml
 *
 * @param parent Parent container (wizard_content)
 * @return Root object of the screen, or nullptr on failure
 */
lv_obj_t* ui_wizard_heater_select_create(lv_obj_t* parent);

/**
 * @brief Cleanup heater select screen resources
 *
 * Saves heater selections to config (both heater and sensor paths)
 * and releases resources
 */
void ui_wizard_heater_select_cleanup();

/**
 * @brief Check if heater selection is complete
 *
 * @return true (always validated for baseline implementation)
 */
bool ui_wizard_heater_select_is_validated();
