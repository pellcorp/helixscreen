// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "display_backend.h"

#include <lvgl.h>
#include <memory>

/**
 * @brief Manages LVGL display initialization and lifecycle
 *
 * Encapsulates display backend creation, LVGL initialization, and input device
 * setup. Extracted from main.cpp init_lvgl() to enable isolated testing and
 * cleaner application startup.
 *
 * Lifecycle:
 * 1. Create DisplayManager instance
 * 2. Call init() with desired configuration
 * 3. Use display(), pointer_input(), keyboard_input() as needed
 * 4. Call shutdown() or let destructor clean up
 *
 * Thread safety: All methods should be called from the main thread.
 *
 * @code
 * DisplayManager display_mgr;
 * DisplayManager::Config config;
 * config.width = 800;
 * config.height = 480;
 *
 * if (!display_mgr.init(config)) {
 *     spdlog::error("Failed to initialize display");
 *     return 1;
 * }
 *
 * // Use display_mgr.display() for LVGL operations
 * // ...
 *
 * display_mgr.shutdown();
 * @endcode
 */
class DisplayManager {
  public:
    /**
     * @brief Display configuration options
     */
    struct Config {
        int width = 800;             ///< Display width in pixels
        int height = 480;            ///< Display height in pixels
        int scroll_throw = 25;       ///< Scroll momentum decay (1-99, higher = faster decay)
        int scroll_limit = 5;        ///< Pixels before scrolling starts
        bool require_pointer = true; ///< Fail init if no pointer device (embedded only)
    };

    DisplayManager();
    ~DisplayManager();

    // Non-copyable, non-movable (owns unique resources)
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    DisplayManager(DisplayManager&&) = delete;
    DisplayManager& operator=(DisplayManager&&) = delete;

    /**
     * @brief Initialize LVGL and display backend
     *
     * Creates display backend (auto-detected), initializes LVGL,
     * creates display and input devices.
     *
     * @param config Display configuration
     * @return true on success, false on failure (logs error details)
     */
    bool init(const Config& config);

    /**
     * @brief Shutdown display and release resources
     *
     * Safe to call multiple times. Called automatically by destructor.
     */
    void shutdown();

    /**
     * @brief Check if display is initialized
     * @return true if init() succeeded and shutdown() not called
     */
    bool is_initialized() const {
        return m_initialized;
    }

    /**
     * @brief Get LVGL display object
     * @return Display pointer, or nullptr if not initialized
     */
    lv_display_t* display() const {
        return m_display;
    }

    /**
     * @brief Get pointer input device (mouse/touch)
     * @return Input device pointer, or nullptr if not available
     */
    lv_indev_t* pointer_input() const {
        return m_pointer;
    }

    /**
     * @brief Get keyboard input device
     * @return Input device pointer, or nullptr if not available
     */
    lv_indev_t* keyboard_input() const {
        return m_keyboard;
    }

    /**
     * @brief Get display backend
     * @return Backend pointer, or nullptr if not initialized
     */
    DisplayBackend* backend() const {
        return m_backend.get();
    }

    /**
     * @brief Get current display width
     * @return Width in pixels, or 0 if not initialized
     */
    int width() const {
        return m_width;
    }

    /**
     * @brief Get current display height
     * @return Height in pixels, or 0 if not initialized
     */
    int height() const {
        return m_height;
    }

    // ========================================================================
    // Static Timing Functions (portable across platforms)
    // ========================================================================

    /**
     * @brief Get current tick count in milliseconds
     *
     * Uses SDL_GetTicks() on desktop, clock_gettime() on embedded.
     *
     * @return Milliseconds since some fixed point (wraps at ~49 days)
     */
    static uint32_t get_ticks();

    /**
     * @brief Delay for specified milliseconds
     *
     * Uses SDL_Delay() on desktop, nanosleep() on embedded.
     *
     * @param ms Milliseconds to delay
     */
    static void delay(uint32_t ms);

  private:
    bool m_initialized = false;
    int m_width = 0;
    int m_height = 0;

    std::unique_ptr<DisplayBackend> m_backend;
    lv_display_t* m_display = nullptr;
    lv_indev_t* m_pointer = nullptr;
    lv_indev_t* m_keyboard = nullptr;
    lv_group_t* m_input_group = nullptr;

    /**
     * @brief Configure scroll behavior on pointer device
     */
    void configure_scroll(int scroll_throw, int scroll_limit);

    /**
     * @brief Set up keyboard input group
     */
    void setup_keyboard_group();
};
