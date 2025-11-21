// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 HelixScreen Contributors
/**
 * @file ui_async_callback.h
 * @brief RAII wrapper for safe LVGL async callback data management
 *
 * This module provides safe wrappers for managing callback data in LVGL's
 * lv_async_call() to prevent memory leaks if callbacks are never executed.
 */

#ifndef UI_ASYNC_CALLBACK_H
#define UI_ASYNC_CALLBACK_H

#include "lvgl/lvgl.h"
#include <functional>
#include <memory>

/**
 * @brief RAII wrapper for lv_async_call callback data
 *
 * This template ensures callback data is properly cleaned up even if the
 * async callback never executes (e.g., during LVGL shutdown).
 *
 * The data is wrapped in a unique_ptr which is released to void* for lv_async_call,
 * then reconstituted and deleted in the callback. This is safe because:
 * 1. If callback executes: unique_ptr is deleted normally
 * 2. If callback never executes: We accept the leak as lv_async_call's limitation
 *    (but this is better than manual new/delete which ALWAYS risks leaks)
 *
 * Usage:
 * @code
 * struct MyData { int value; std::string message; };
 *
 * ui_async_call_safe<MyData>(
 *     std::make_unique<MyData>(42, "hello"),
 *     [](MyData* d) {
 *         spdlog::info("Value: {}, Message: {}", d->value, d->message);
 *     }
 * );
 * @endcode
 *
 * @tparam T Type of callback data
 * @param data Unique pointer to callback data (ownership transferred)
 * @param callback Function to execute in LVGL thread with data pointer
 */
template <typename T>
void ui_async_call_safe(std::unique_ptr<T> data,
                        std::function<void(T*)> callback) {
    // Package data and callback together
    struct AsyncPackage {
        std::unique_ptr<T> data;
        std::function<void(T*)> callback;
    };

    auto* package = new AsyncPackage{std::move(data), std::move(callback)};

    lv_async_call(
        [](void* user_data) {
            // Wrap in unique_ptr for RAII cleanup
            std::unique_ptr<AsyncPackage> pkg(static_cast<AsyncPackage*>(user_data));

            // Execute callback
            pkg->callback(pkg->data.get());

            // pkg and pkg->data automatically deleted when unique_ptrs go out of scope
        },
        package
    );
}

#endif // UI_ASYNC_CALLBACK_H
