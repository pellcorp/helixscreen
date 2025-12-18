// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl/lvgl.h"

#include <utility>

/**
 * @file ui_timer_guard.h
 * @brief RAII wrapper for LVGL timers - auto-deletes on destruction
 *
 * LVGL timers created with lv_timer_create() are NOT automatically cleaned up
 * when their user_data object is destroyed. They continue running with
 * dangling pointers until explicitly deleted with lv_timer_delete().
 *
 * This wrapper ensures timers are properly deleted when the guard goes out
 * of scope, preventing use-after-free crashes.
 *
 * @code
 * class MyPanel {
 *     LvglTimerGuard update_timer_;
 *
 *     void start_updates() {
 *         update_timer_.reset(lv_timer_create(update_cb, 1000, this));
 *     }
 *     // Timer automatically deleted when MyPanel is destroyed
 * };
 * @endcode
 *
 * @see ObserverGuard for similar pattern with LVGL observers
 */
class LvglTimerGuard {
  public:
    LvglTimerGuard() = default;

    explicit LvglTimerGuard(lv_timer_t* timer) : timer_(timer) {}

    ~LvglTimerGuard() {
        reset();
    }

    LvglTimerGuard(LvglTimerGuard&& other) noexcept
        : timer_(std::exchange(other.timer_, nullptr)) {}

    LvglTimerGuard& operator=(LvglTimerGuard&& other) noexcept {
        if (this != &other) {
            reset();
            timer_ = std::exchange(other.timer_, nullptr);
        }
        return *this;
    }

    LvglTimerGuard(const LvglTimerGuard&) = delete;
    LvglTimerGuard& operator=(const LvglTimerGuard&) = delete;

    /**
     * @brief Delete current timer and optionally set a new one
     *
     * Safe to call during static destruction - checks lv_is_initialized()
     * to avoid crash if LVGL has already shut down.
     *
     * @param new_timer New timer to manage, or nullptr to just delete current
     */
    void reset(lv_timer_t* new_timer = nullptr) {
        if (timer_ && lv_is_initialized()) {
            lv_timer_delete(timer_);
        }
        timer_ = new_timer;
    }

    /**
     * @brief Release ownership without deleting the timer
     *
     * Use when transferring ownership or when the timer is self-deleting
     * (one-shot timers that call lv_timer_delete() in their callback).
     *
     * @return The timer pointer (caller takes ownership)
     */
    lv_timer_t* release() {
        return std::exchange(timer_, nullptr);
    }

    /**
     * @brief Check if a timer is being managed
     */
    explicit operator bool() const {
        return timer_ != nullptr;
    }

    /**
     * @brief Get the managed timer
     */
    lv_timer_t* get() const {
        return timer_;
    }

  private:
    lv_timer_t* timer_ = nullptr;
};
