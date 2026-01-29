// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file ui_cleanup_helpers.h
 * @brief RAII-style safe deletion helpers for LVGL objects and timers
 *
 * These helpers eliminate the repetitive if-delete-null pattern found in
 * panel destructors. Each helper safely checks for null, deletes the resource,
 * and nulls the pointer to prevent double-free.
 *
 * @code{.cpp}
 * // Before (repeated 7+ times per panel):
 * if (overlay_cache_) {
 *     lv_obj_del(overlay_cache_);
 *     overlay_cache_ = nullptr;
 * }
 *
 * // After:
 * safe_delete_obj(overlay_cache_);
 * @endcode
 */

#include "lvgl.h"

namespace helix::ui {

/**
 * @brief Safely delete an LVGL object and null the pointer
 * @param obj Reference to object pointer (will be nulled after deletion)
 *
 * Safe to call with nullptr - no-op in that case.
 * Prevents double-free by nulling pointer after deletion.
 */
inline void safe_delete_obj(lv_obj_t*& obj) {
    if (obj) {
        lv_obj_del(obj);
        obj = nullptr;
    }
}

/**
 * @brief Safely delete an LVGL timer and null the pointer
 * @param timer Reference to timer pointer (will be nulled after deletion)
 *
 * Safe to call with nullptr - no-op in that case.
 * Prevents double-free by nulling pointer after deletion.
 */
inline void safe_delete_timer(lv_timer_t*& timer) {
    if (timer) {
        lv_timer_delete(timer);
        timer = nullptr;
    }
}

} // namespace helix::ui
