// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui_ams_mini_status.h"

#include "ui_fonts.h"
#include "ui_theme.h"

#include "lvgl/src/xml/lv_xml.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstring>
#include <unordered_map>

// ============================================================================
// Layout constants
// ============================================================================

/** Minimum fill height in pixels (ensures visibility when present) */
static constexpr int32_t MIN_FILL_HEIGHT_PX = 2;

/** Minimum bar width in pixels (prevents bars from becoming invisible) */
static constexpr int32_t MIN_BAR_WIDTH_PX = 3;

/** Border radius for bar corners in pixels */
static constexpr int32_t BAR_BORDER_RADIUS_PX = 2;

// ============================================================================
// Per-widget user data
// ============================================================================

/** Magic number to identify ams_mini_status widgets ("AMS1" as ASCII) */
static constexpr uint32_t AMS_MINI_STATUS_MAGIC = 0x414D5331;

/**
 * @brief Per-slot data stored for each bar
 */
struct SlotBarData {
    lv_obj_t* bar_bg = nullptr;   // Background (gray when empty)
    lv_obj_t* bar_fill = nullptr; // Fill portion (colored)
    uint32_t color_rgb = 0x808080;
    int fill_pct = 100;
    bool present = false;
};

/**
 * @brief User data stored on each ams_mini_status widget
 */
struct AmsMiniStatusData {
    uint32_t magic = AMS_MINI_STATUS_MAGIC;
    int32_t height = 32;
    int gate_count = 0;
    int max_visible = AMS_MINI_STATUS_MAX_VISIBLE;

    // Child objects
    lv_obj_t* container = nullptr;      // Main container
    lv_obj_t* bars_container = nullptr; // Container for slot bars
    lv_obj_t* overflow_label = nullptr; // "+N" overflow indicator

    // Per-slot data
    SlotBarData slots[AMS_MINI_STATUS_MAX_VISIBLE];
};

// Static registry for safe cleanup
static std::unordered_map<lv_obj_t*, AmsMiniStatusData*> s_registry;

static AmsMiniStatusData* get_data(lv_obj_t* obj) {
    auto it = s_registry.find(obj);
    return (it != s_registry.end()) ? it->second : nullptr;
}

// ============================================================================
// Internal helpers
// ============================================================================

/** Calculate bar width based on container width and slot count */
static int32_t calc_bar_width(int32_t container_width, int visible_count, int32_t gap) {
    if (visible_count <= 0)
        return 0;
    int32_t total_gaps = (visible_count - 1) * gap;
    return (container_width - total_gaps) / visible_count;
}

/** Update a single slot bar's appearance */
static void update_slot_bar(SlotBarData* slot, int32_t height) {
    if (!slot->bar_bg || !slot->bar_fill)
        return;

    // Background: use theme colors - darker when present, dimmed when empty
    if (slot->present) {
        lv_obj_set_style_bg_color(slot->bar_bg, ui_theme_get_color("panel_bg"), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slot->bar_bg, LV_OPA_COVER, LV_PART_MAIN);
    } else {
        lv_obj_set_style_bg_color(slot->bar_bg, ui_theme_get_color("panel_bg"), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slot->bar_bg, LV_OPA_50, LV_PART_MAIN);
    }

    // Fill: colored portion from bottom
    if (slot->present && slot->fill_pct > 0) {
        lv_obj_set_style_bg_color(slot->bar_fill, lv_color_hex(slot->color_rgb), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(slot->bar_fill, LV_OPA_COVER, LV_PART_MAIN);

        // Height based on fill percentage
        int32_t fill_height = (height * slot->fill_pct) / 100;
        fill_height = std::max(MIN_FILL_HEIGHT_PX, fill_height);
        lv_obj_set_height(slot->bar_fill, fill_height);
        lv_obj_remove_flag(slot->bar_fill, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(slot->bar_fill, LV_OBJ_FLAG_HIDDEN);
    }
}

/** Rebuild the bars based on gate_count and max_visible */
static void rebuild_bars(AmsMiniStatusData* data) {
    if (!data || !data->bars_container)
        return;

    int visible_count = std::min(data->gate_count, data->max_visible);
    int overflow_count = data->gate_count - visible_count;

    // Get bar dimensions using responsive spacing
    lv_obj_update_layout(data->container);
    int32_t container_width = lv_obj_get_content_width(data->bars_container);
    if (container_width <= 0) {
        spdlog::warn("[AmsMiniStatus] Container has zero width, skipping rebuild");
        return;
    }
    int32_t gap = ui_theme_get_spacing("space_xxs"); // Responsive 2-4px gap
    int32_t bar_width = calc_bar_width(container_width, visible_count, gap);
    bar_width = std::max(MIN_BAR_WIDTH_PX, bar_width);

    // Create/update bars
    for (int i = 0; i < AMS_MINI_STATUS_MAX_VISIBLE; i++) {
        SlotBarData* slot = &data->slots[i];

        if (i < visible_count) {
            // Show this slot
            if (!slot->bar_bg) {
                // Create bar background
                slot->bar_bg = lv_obj_create(data->bars_container);
                lv_obj_remove_flag(slot->bar_bg, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_border_width(slot->bar_bg, 0, LV_PART_MAIN);
                lv_obj_set_style_pad_all(slot->bar_bg, 0, LV_PART_MAIN);
                lv_obj_set_style_radius(slot->bar_bg, BAR_BORDER_RADIUS_PX, LV_PART_MAIN);

                // Create fill inside
                slot->bar_fill = lv_obj_create(slot->bar_bg);
                lv_obj_remove_flag(slot->bar_fill, LV_OBJ_FLAG_SCROLLABLE);
                lv_obj_set_style_border_width(slot->bar_fill, 0, LV_PART_MAIN);
                lv_obj_set_style_pad_all(slot->bar_fill, 0, LV_PART_MAIN);
                lv_obj_set_style_radius(slot->bar_fill, BAR_BORDER_RADIUS_PX, LV_PART_MAIN);
                lv_obj_set_width(slot->bar_fill, LV_PCT(100));
                lv_obj_set_align(slot->bar_fill, LV_ALIGN_BOTTOM_MID);
            }

            lv_obj_set_size(slot->bar_bg, bar_width, data->height);
            lv_obj_remove_flag(slot->bar_bg, LV_OBJ_FLAG_HIDDEN);
            update_slot_bar(slot, data->height);
        } else {
            // Hide this slot
            if (slot->bar_bg) {
                lv_obj_add_flag(slot->bar_bg, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }

    // Update overflow label
    if (data->overflow_label) {
        if (overflow_count > 0) {
            char buf[8];
            snprintf(buf, sizeof(buf), "+%d", overflow_count);
            lv_label_set_text(data->overflow_label, buf);
            lv_obj_remove_flag(data->overflow_label, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(data->overflow_label, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Hide entire widget if no gates
    if (data->gate_count <= 0) {
        lv_obj_add_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(data->container, LV_OBJ_FLAG_HIDDEN);
    }
}

/** Cleanup callback when widget is deleted */
static void on_delete(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target_obj(e);
    auto it = s_registry.find(obj);
    if (it != s_registry.end()) {
        delete it->second;
        s_registry.erase(it);
    }
}

// ============================================================================
// Public API
// ============================================================================

lv_obj_t* ui_ams_mini_status_create(lv_obj_t* parent, int32_t height) {
    if (!parent || height <= 0) {
        return nullptr;
    }

    // Create main container
    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_remove_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);

    // Horizontal layout for bars + overflow label
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(container, ui_theme_get_spacing("space_xs"), LV_PART_MAIN);

    lv_obj_set_size(container, LV_SIZE_CONTENT, height);

    // Create user data
    auto* data = new AmsMiniStatusData();
    data->height = height;
    data->container = container;

    // Create bars container (holds the slot bars)
    data->bars_container = lv_obj_create(container);
    lv_obj_remove_flag(data->bars_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(data->bars_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(data->bars_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(data->bars_container, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(data->bars_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(data->bars_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(data->bars_container, ui_theme_get_spacing("space_xxs"),
                                LV_PART_MAIN);
    lv_obj_set_size(data->bars_container, LV_SIZE_CONTENT, height);

    // Create overflow label (hidden by default) - use responsive font
    data->overflow_label = lv_label_create(container);
    lv_label_set_text(data->overflow_label, "+0");
    lv_obj_set_style_text_color(data->overflow_label, ui_theme_get_color("text_secondary"),
                                LV_PART_MAIN);
    const char* font_xs_name = lv_xml_get_const(nullptr, "font_xs");
    const lv_font_t* font_xs =
        font_xs_name ? lv_xml_get_font(nullptr, font_xs_name) : &noto_sans_12;
    lv_obj_set_style_text_font(data->overflow_label, font_xs, LV_PART_MAIN);
    lv_obj_add_flag(data->overflow_label, LV_OBJ_FLAG_HIDDEN);

    // Register and set up cleanup
    s_registry[container] = data;
    lv_obj_add_event_cb(container, on_delete, LV_EVENT_DELETE, nullptr);

    // Initially hidden (no gates)
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);

    spdlog::debug("[AmsMiniStatus] Created (height={})", height);
    return container;
}

void ui_ams_mini_status_set_gate_count(lv_obj_t* obj, int gate_count) {
    auto* data = get_data(obj);
    if (!data)
        return;

    gate_count = std::max(0, gate_count);
    if (data->gate_count == gate_count)
        return;

    data->gate_count = gate_count;
    rebuild_bars(data);

    spdlog::debug("[AmsMiniStatus] gate_count={}", gate_count);
}

void ui_ams_mini_status_set_max_visible(lv_obj_t* obj, int max_visible) {
    auto* data = get_data(obj);
    if (!data)
        return;

    max_visible = std::max(1, std::min(AMS_MINI_STATUS_MAX_VISIBLE, max_visible));
    if (data->max_visible == max_visible)
        return;

    data->max_visible = max_visible;
    rebuild_bars(data);
}

void ui_ams_mini_status_set_slot(lv_obj_t* obj, int slot_index, uint32_t color_rgb, int fill_pct,
                                 bool present) {
    auto* data = get_data(obj);
    if (!data || slot_index < 0 || slot_index >= AMS_MINI_STATUS_MAX_VISIBLE)
        return;

    SlotBarData* slot = &data->slots[slot_index];
    slot->color_rgb = color_rgb;
    slot->fill_pct = std::max(0, std::min(100, fill_pct));
    slot->present = present;

    update_slot_bar(slot, data->height);
}

void ui_ams_mini_status_refresh(lv_obj_t* obj) {
    auto* data = get_data(obj);
    if (!data)
        return;

    rebuild_bars(data);
}

bool ui_ams_mini_status_is_valid(lv_obj_t* obj) {
    auto* data = get_data(obj);
    return data && data->magic == AMS_MINI_STATUS_MAGIC;
}
