// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize HelixScreen custom theme
 *
 * Creates a wrapper theme that delegates to LVGL default theme but overrides
 * input widget backgrounds to use a different color than cards. This gives
 * input widgets (textarea, dropdown) visual distinction from card backgrounds.
 *
 * Color computation:
 * - Dark mode: Input bg = card bg + (22, 23, 27) RGB offset (lighter)
 * - Light mode: Input bg = card bg - (22, 23, 27) RGB offset (darker)
 *
 * The theme reads all colors from globals.xml via lv_xml_get_const(), ensuring
 * no hardcoded colors in C code.
 *
 * @param display LVGL display to apply theme to
 * @param primary_color Primary theme color (from globals.xml)
 * @param secondary_color Secondary theme color (from globals.xml)
 * @param text_primary_color Primary text color for buttons/labels (theme-aware)
 * @param is_dark Dark mode flag (true = dark mode)
 * @param base_font Base font for theme
 * @param screen_bg Screen background color (from globals.xml variant)
 * @param card_bg Card background color (from globals.xml variant)
 * @param surface_control Control surface color for buttons/inputs (mode-aware)
 * @param focus_color Focus ring color for accessibility
 * @param border_color Border/outline color for slider tracks and buttons
 * @param border_radius Border radius for buttons/cards (from theme)
 * @param border_width Border width for buttons (from theme)
 * @param knob_color Color for slider/switch knobs (brighter of primary/tertiary)
 * @param accent_color Color for checkbox checkmark (more saturated of primary/secondary)
 * @return Initialized theme, or NULL on failure
 *
 * Example usage:
 * @code
 *   lv_color_t primary = theme_manager_parse_hex_color("#FF4444");
 *   lv_color_t screen_bg = theme_manager_get_color("app_bg_color");
 *   int32_t border_radius = atoi(lv_xml_get_const(NULL, "border_radius"));
 *   lv_theme_t* theme = theme_core_init(
 *       display, primary, secondary, true, font, screen_bg, card_bg, grey, border_radius
 *   );
 *   lv_display_set_theme(display, theme);
 * @endcode
 */
lv_theme_t* theme_core_init(lv_display_t* display, lv_color_t primary_color,
                            lv_color_t secondary_color, lv_color_t text_primary_color,
                            lv_color_t text_muted_color, lv_color_t text_subtle_color, bool is_dark,
                            const lv_font_t* base_font, lv_color_t screen_bg, lv_color_t card_bg,
                            lv_color_t surface_control, lv_color_t focus_color,
                            lv_color_t border_color, int32_t border_radius, int32_t border_width,
                            int32_t border_opacity, lv_color_t knob_color, lv_color_t accent_color);

/**
 * @brief Update theme colors in-place without recreating the theme
 *
 * Updates all theme style objects with new colors for runtime dark/light mode
 * switching. This modifies existing styles and calls lv_obj_report_style_change()
 * to trigger LVGL's style refresh cascade.
 *
 * Unlike theme_core_init(), this function preserves widget state and avoids
 * the overhead of theme recreation.
 *
 * @param is_dark true for dark mode colors, false for light mode
 * @param screen_bg Screen background color
 * @param card_bg Card/panel background color
 * @param surface_control Control surface color for buttons/inputs
 * @param text_primary_color Primary text color
 * @param focus_color Focus ring color for accessibility
 * @param primary_color Primary accent color (unused, kept for compatibility)
 * @param secondary_color Secondary accent color for slider/switch indicators
 * @param border_color Border color for slider tracks
 * @param knob_color Color for slider/switch knobs (brighter of secondary/tertiary)
 * @param accent_color Color for checkbox checkmark (more saturated of primary/secondary)
 */
void theme_core_update_colors(bool is_dark, lv_color_t screen_bg, lv_color_t card_bg,
                              lv_color_t surface_control, lv_color_t text_primary_color,
                              lv_color_t text_muted_color, lv_color_t text_subtle_color,
                              lv_color_t focus_color, lv_color_t primary_color,
                              lv_color_t secondary_color, lv_color_t border_color,
                              int32_t border_opacity, lv_color_t knob_color,
                              lv_color_t accent_color);

/**
 * @brief Update all theme colors for live preview
 *
 * Updates theme styles in-place without requiring restart.
 * Call lv_obj_report_style_change(NULL) after to trigger refresh.
 *
 * @param is_dark Dark mode flag
 * @param colors Array of 16 hex color strings (palette order)
 * @param border_radius Corner radius in pixels
 */
void theme_core_preview_colors(bool is_dark, const char* colors[16], int32_t border_radius,
                               int32_t border_opacity);

/**
 * @brief Get the shared card style
 *
 * Returns a pointer to the persistent card style that includes:
 * - bg_color: card_bg token
 * - bg_opa: LV_OPA_COVER
 * - border_color, border_width, border_opa
 * - radius: from border_radius parameter
 *
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to card style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_card_style(void);

/**
 * @brief Get the shared dialog style
 *
 * Returns a pointer to the persistent dialog style that includes:
 * - bg_color: surface_control/card_alt token
 * - bg_opa: LV_OPA_COVER
 * - radius: from border_radius parameter
 *
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to dialog style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_dialog_style(void);

/**
 * @brief Get the shared primary text style
 *
 * Returns a pointer to the persistent text style that includes:
 * - text_color: text_primary_color token
 *
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to text style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_text_style(void);

/**
 * @brief Get the shared muted text style
 *
 * Returns a pointer to the persistent muted text style that includes:
 * - text_color: text_primary_color with reduced opacity
 * - text_opa: ~70% for muted appearance
 *
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to muted text style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_text_muted_style(void);

/**
 * @brief Get the shared subtle text style
 *
 * Returns a pointer to the persistent subtle text style that includes:
 * - text_color: text_subtle_color (even more muted than text_muted)
 *
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to subtle text style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_text_subtle_style(void);

// ============================================================================
// Icon Style Getters (Phase 2.1)
// ============================================================================
// Icon styles mirror text styles but for icon coloring. Icons in LVGL are
// font-based labels, so they use text_color for their color.

/**
 * @brief Get the shared icon text style
 *
 * Returns a pointer to the persistent icon style using text_primary_color.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon text style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_text_style(void);

/**
 * @brief Get the shared icon muted style
 *
 * Returns a pointer to the persistent icon style using text_muted_color.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon muted style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_muted_style(void);

/**
 * @brief Get the shared icon primary style
 *
 * Returns a pointer to the persistent icon style using primary_color (accent).
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon primary style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_primary_style(void);

/**
 * @brief Get the shared icon secondary style
 *
 * Returns a pointer to the persistent icon style using secondary_color.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon secondary style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_secondary_style(void);

/**
 * @brief Get the shared icon tertiary style
 *
 * Returns a pointer to the persistent icon style using text_subtle_color.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon tertiary style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_tertiary_style(void);

/**
 * @brief Get the shared icon success style
 *
 * Returns a pointer to the persistent icon style for success state.
 * Uses a green success color.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon success style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_success_style(void);

/**
 * @brief Get the shared icon warning style
 *
 * Returns a pointer to the persistent icon style for warning state.
 * Uses an amber/orange warning color.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon warning style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_warning_style(void);

/**
 * @brief Get the shared icon danger style
 *
 * Returns a pointer to the persistent icon style for danger/error state.
 * Uses a red danger color.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon danger style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_danger_style(void);

/**
 * @brief Get the shared icon info style
 *
 * Returns a pointer to the persistent icon style for info state.
 * Uses a blue info color.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to icon info style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_icon_info_style(void);

// ============================================================================
// Spinner Style Getters (Phase 2.3)
// ============================================================================

/**
 * @brief Get the shared spinner style
 *
 * Returns a pointer to the persistent spinner style using primary_color for arc.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to spinner style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_spinner_style(void);

// ============================================================================
// Severity Style Getters (Phase 2.3)
// ============================================================================
// Severity styles are for severity_card border colors. Each severity level
// (info, success, warning, danger) has its own style with border_color set.

/**
 * @brief Get the shared severity info style
 * @return Pointer to severity info style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_severity_info_style(void);

/**
 * @brief Get the shared severity success style
 * @return Pointer to severity success style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_severity_success_style(void);

/**
 * @brief Get the shared severity warning style
 * @return Pointer to severity warning style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_severity_warning_style(void);

/**
 * @brief Get the shared severity danger style
 * @return Pointer to severity danger style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_severity_danger_style(void);

// ============================================================================
// Button Style Getters (Phase 2.6a)
// ============================================================================
// Button styles provide reactive background colors for different button types.
// Each style sets bg_color only - text color is handled separately by the
// button widget using contrast text getters.

/**
 * @brief Get the shared button primary style
 *
 * Returns a pointer to the persistent button style using primary_color for bg.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to button primary style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_button_primary_style(void);

/**
 * @brief Get the shared button secondary style
 *
 * Returns a pointer to the persistent button style using surface_control for bg.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to button secondary style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_button_secondary_style(void);

/**
 * @brief Get the shared button danger style
 *
 * Returns a pointer to the persistent button style using danger color for bg.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to button danger style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_button_danger_style(void);

/**
 * @brief Get the shared button ghost style
 *
 * Returns a pointer to the persistent button style with transparent background.
 * The style updates in-place when theme_core_update_colors() is called.
 *
 * @return Pointer to button ghost style, or NULL if theme not initialized
 */
lv_style_t* theme_core_get_button_ghost_style(void);

// ============================================================================
// Contrast Text Color Getters (Phase 2.6a)
// ============================================================================
// Contrast text getters provide appropriate text colors for dark and light
// backgrounds. These are used by button widgets to pick readable text colors
// based on background luminance.

/**
 * @brief Get text color appropriate for dark backgrounds
 *
 * Returns a light text color (near-white) suitable for display on dark
 * backgrounds. Used by button widgets to ensure text readability.
 *
 * @return Light text color for dark backgrounds (fallback: white 0xFFFFFF)
 */
lv_color_t theme_core_get_text_for_dark_bg(void);

/**
 * @brief Get text color appropriate for light backgrounds
 *
 * Returns a dark text color suitable for display on light backgrounds.
 * Used by button widgets to ensure text readability.
 *
 * @return Dark text color for light backgrounds (fallback: dark gray 0x212121)
 */
lv_color_t theme_core_get_text_for_light_bg(void);

#ifdef __cplusplus
}
#endif
