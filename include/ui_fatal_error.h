// SPDX-License-Identifier: GPL-3.0-or-later
//
// HelixScreen - Fatal Error Display

#ifndef __HELIX_UI_FATAL_ERROR_H__
#define __HELIX_UI_FATAL_ERROR_H__

#include <cstdint>

/**
 * @brief Display a fatal error screen for critical startup failures
 *
 * Used for critical startup errors on embedded systems where the UI cannot function
 * (e.g., no input device found on a touchscreen-only device, display init failure).
 *
 * This function creates a simple LVGL-based error screen with:
 * - Red background to indicate error
 * - Title and detailed message
 * - Troubleshooting suggestions
 *
 * @param title Error title (e.g., "Input Device Error")
 * @param message Detailed error message
 * @param suggestions Array of troubleshooting suggestions (null-terminated)
 * @param display_ms How long to show the error (0 = indefinite until kill)
 */
void ui_show_fatal_error(const char* title, const char* message,
                         const char* const* suggestions, uint32_t display_ms);

#endif // __HELIX_UI_FATAL_ERROR_H__
