// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @brief Manages font and image registration with LVGL XML system
 *
 * Provides static methods for registering fonts and images that can be
 * referenced by name in XML layout files. Extracted from main.cpp
 * register_fonts_and_images() to enable isolated testing.
 *
 * All methods are static since assets are registered globally with LVGL.
 * Registration is idempotent - calling multiple times is safe.
 *
 * @code
 * // Register all assets at startup
 * AssetManager::register_all();
 *
 * // Or register separately
 * AssetManager::register_fonts();
 * AssetManager::register_images();
 * @endcode
 */
class AssetManager {
  public:
    /**
     * @brief Register all fonts with LVGL XML system
     *
     * Registers:
     * - MDI icon fonts (16, 24, 32, 48, 64px)
     * - Noto Sans regular fonts (10-28px)
     * - Noto Sans bold fonts (14-28px)
     * - Montserrat aliases (for XML compatibility)
     */
    static void register_fonts();

    /**
     * @brief Register all images with LVGL XML system
     *
     * Registers common images used in XML layouts:
     * - Printer placeholder images
     * - Filament spool graphics
     * - Thumbnail placeholders
     * - SVG icons
     */
    static void register_images();

    /**
     * @brief Register all assets (fonts and images)
     *
     * Convenience method that calls register_fonts() and register_images().
     */
    static void register_all();

    /**
     * @brief Check if fonts have been registered
     * @return true if register_fonts() has been called
     */
    static bool fonts_registered();

    /**
     * @brief Check if images have been registered
     * @return true if register_images() has been called
     */
    static bool images_registered();

  private:
    static bool s_fonts_registered;
    static bool s_images_registered;
};
