// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "printer_detector.h"
#include "printer_types.h"

#include <filesystem>
#include <sstream>
#include <string>

/**
 * @file printer_images.h
 * @brief Printer type to image path mapping
 *
 * Provides image path lookups for printer types using the unified printer database
 * (config/printer_database.json). Falls back to generic CoreXY image when a printer
 * image is not found or the file doesn't exist on disk.
 *
 * Image paths are stored in the database as just filenames (e.g., "voron-24r2.png").
 * This header adds the full LVGL path prefix "A:assets/images/printers/".
 */

namespace PrinterImages {

/// Base path for printer images (LVGL filesystem prefix)
inline constexpr const char* IMAGE_BASE_PATH = "A:assets/images/printers/";

/// Default fallback image for unknown/unmapped printers (generic CoreXY)
inline constexpr const char* DEFAULT_IMAGE = "A:assets/images/printers/generic-corexy.png";

/// Default image filename (without path)
inline constexpr const char* DEFAULT_IMAGE_FILENAME = "generic-corexy.png";

/**
 * @brief Get printer name from type index
 *
 * @param printer_type_index Index from printer type roller (0-39)
 * @return Printer name string (e.g., "Voron 2.4")
 */
inline std::string get_printer_name(int printer_type_index) {
    std::istringstream stream(PrinterTypes::PRINTER_TYPES_ROLLER);
    std::string line;
    int index = 0;

    while (std::getline(stream, line)) {
        if (index == printer_type_index) {
            return line;
        }
        ++index;
    }
    return "Unknown";
}

/**
 * @brief Convert LVGL path (A:...) to filesystem path
 *
 * @param lvgl_path Path with "A:" prefix
 * @return Filesystem path without prefix
 */
inline std::string lvgl_to_fs_path(const char* lvgl_path) {
    if (lvgl_path && lvgl_path[0] == 'A' && lvgl_path[1] == ':') {
        return std::string(lvgl_path + 2); // Skip "A:" prefix
    }
    return lvgl_path ? lvgl_path : "";
}

/**
 * @brief Check if a file exists at the given LVGL path
 *
 * @param lvgl_path Path with "A:" prefix
 * @return true if file exists, false otherwise
 */
inline bool image_file_exists(const std::string& lvgl_path) {
    std::string fs_path = lvgl_to_fs_path(lvgl_path.c_str());
    return !fs_path.empty() && std::filesystem::exists(fs_path);
}

/**
 * @brief Get image path for a printer name (from database)
 *
 * Looks up the image in the printer database JSON and constructs the full
 * LVGL path. Falls back to DEFAULT_IMAGE if not found or file doesn't exist.
 *
 * @param printer_name Printer name (e.g., "Voron 2.4", "FlashForge Adventurer 5M")
 * @return Full LVGL path to printer image
 */
inline std::string get_image_path_for_name(const std::string& printer_name) {
    // Look up image filename from database
    std::string image_filename = PrinterDetector::get_image_for_printer(printer_name);

    if (!image_filename.empty()) {
        std::string full_path = std::string(IMAGE_BASE_PATH) + image_filename;

        // Verify file exists
        if (image_file_exists(full_path)) {
            return full_path;
        }
    }

    // Fall back to default
    return DEFAULT_IMAGE;
}

/**
 * @brief Get image path for a printer type index
 *
 * Converts index to printer name, then looks up image in database.
 * Falls back to DEFAULT_IMAGE if not found or file doesn't exist.
 *
 * @param printer_type_index Index from printer type roller (0-39)
 * @return Full LVGL path to printer image
 */
inline std::string get_image_path(int printer_type_index) {
    std::string printer_name = get_printer_name(printer_type_index);
    return get_image_path_for_name(printer_name);
}

/**
 * @brief Get validated image path for a printer type, with fallback
 *
 * This is the primary function to use for displaying printer images.
 * It handles all lookup and validation logic internally.
 *
 * @param printer_type_index Index from printer type roller (0-39)
 * @return Full LVGL path to printer image (guaranteed to exist or be default)
 *
 * @note Returns a std::string that manages its own memory. The caller
 *       must keep the string alive while using the path.
 */
inline std::string get_validated_image_path(int printer_type_index) {
    return get_image_path(printer_type_index);
}

} // namespace PrinterImages
