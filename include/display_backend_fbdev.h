// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later
//
// HelixScreen - Linux Framebuffer Display Backend
//
// Embedded Linux backend using /dev/fb0 for direct framebuffer access.
// Used for AD5M and as fallback on Raspberry Pi.

#pragma once

#ifdef HELIX_DISPLAY_FBDEV

#include "display_backend.h"
#include "touch_calibration.h"

#include <string>

/**
 * @brief Calibration context stored in indev user_data
 *
 * Contains both the calibration coefficients and the original read callback
 * so we can chain to it after applying our transform.
 */
struct CalibrationContext {
    helix::TouchCalibration calibration;
    lv_indev_read_cb_t original_read_cb = nullptr;
    int screen_width = 800;
    int screen_height = 480;
};

/**
 * @brief Linux framebuffer display backend for embedded systems
 *
 * Uses LVGL's Linux framebuffer driver (lv_linux_fbdev_create) to
 * render directly to /dev/fb0 without X11/Wayland.
 *
 * Features:
 * - Direct framebuffer access (no compositor overhead)
 * - Works on minimal embedded Linux systems
 * - Touch input via evdev (/dev/input/eventN)
 * - Automatic display size detection from fb0
 *
 * Requirements:
 * - /dev/fb0 must exist and be accessible
 * - Touch device at /dev/input/eventN (configurable)
 */
class DisplayBackendFbdev : public DisplayBackend {
  public:
    /**
     * @brief Construct framebuffer backend with default paths
     *
     * Defaults:
     * - Framebuffer: /dev/fb0
     * - Touch device: auto-detect or /dev/input/event0
     */
    DisplayBackendFbdev();

    /**
     * @brief Construct framebuffer backend with custom paths
     *
     * @param fb_device Path to framebuffer device (e.g., "/dev/fb0")
     * @param touch_device Path to touch input device (e.g., "/dev/input/event4")
     */
    DisplayBackendFbdev(const std::string& fb_device, const std::string& touch_device);

    ~DisplayBackendFbdev() override = default;

    // Display creation
    lv_display_t* create_display(int width, int height) override;

    // Input device creation
    lv_indev_t* create_input_pointer() override;

    // Backend info
    DisplayBackendType type() const override {
        return DisplayBackendType::FBDEV;
    }
    const char* name() const override {
        return "Linux Framebuffer";
    }
    bool is_available() const override;

    // Framebuffer operations
    bool clear_framebuffer(uint32_t color) override;

    // Configuration
    void set_fb_device(const std::string& path) {
        fb_device_ = path;
    }
    void set_touch_device(const std::string& path) {
        touch_device_ = path;
    }

    /**
     * @brief Apply touch calibration at runtime
     *
     * Sets the affine transform coefficients used to convert raw touch
     * coordinates to screen coordinates. Called by the calibration wizard
     * after the user accepts calibration.
     *
     * @param cal Valid calibration coefficients
     * @return true if applied successfully, false if validation failed
     */
    bool set_calibration(const helix::TouchCalibration& cal);

    /**
     * @brief Get current touch calibration
     * @return Current calibration coefficients (may be invalid if not calibrated)
     */
    const helix::TouchCalibration& get_calibration() const {
        return calibration_;
    }

  private:
    std::string fb_device_ = "/dev/fb0";
    std::string touch_device_; // Empty = auto-detect
    lv_display_t* display_ = nullptr;
    lv_indev_t* touch_ = nullptr;

    /// Affine touch calibration coefficients
    helix::TouchCalibration calibration_;

    /// Screen dimensions for coordinate clamping
    int screen_width_ = 800;
    int screen_height_ = 480;

    /// Calibration context for touch input (member to avoid memory leak)
    CalibrationContext calibration_context_;

    /**
     * @brief Auto-detect touch input device
     *
     * Scans /dev/input/event* for touch-capable devices.
     * Falls back to /dev/input/event0 if detection fails.
     *
     * @return Path to touch device
     */
    std::string auto_detect_touch_device() const;
};

#endif // HELIX_DISPLAY_FBDEV
