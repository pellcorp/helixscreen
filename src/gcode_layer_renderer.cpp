// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 HelixScreen Contributors

#include "gcode_layer_renderer.h"

#include "ui_theme.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace helix {
namespace gcode {

// ============================================================================
// Construction
// ============================================================================

GCodeLayerRenderer::GCodeLayerRenderer() {
    // Initialize default colors from theme
    reset_colors();
}

GCodeLayerRenderer::~GCodeLayerRenderer() {
    destroy_cache();
    destroy_ghost_cache();
}

// ============================================================================
// Data Source
// ============================================================================

void GCodeLayerRenderer::set_gcode(const ParsedGCodeFile* gcode) {
    gcode_ = gcode;
    bounds_valid_ = false;
    current_layer_ = 0;
    invalidate_cache();

    if (gcode_) {
        spdlog::debug("[GCodeLayerRenderer] Set G-code: {} layers, {} total segments",
                      gcode_->layers.size(), gcode_->total_segments);
    }
}

// ============================================================================
// Layer Selection
// ============================================================================

void GCodeLayerRenderer::set_current_layer(int layer) {
    if (!gcode_) {
        current_layer_ = 0;
        return;
    }

    // Clamp to valid range
    int max_layer = static_cast<int>(gcode_->layers.size()) - 1;
    current_layer_ = std::clamp(layer, 0, std::max(0, max_layer));
}

int GCodeLayerRenderer::get_layer_count() const {
    return gcode_ ? static_cast<int>(gcode_->layers.size()) : 0;
}

// ============================================================================
// Canvas Setup
// ============================================================================

void GCodeLayerRenderer::set_canvas_size(int width, int height) {
    // Ensure minimum dimensions to prevent division by zero in auto_fit()
    canvas_width_ = std::max(1, width);
    canvas_height_ = std::max(1, height);
    bounds_valid_ = false; // Recalculate fit on next render
}

// ============================================================================
// Colors
// ============================================================================

void GCodeLayerRenderer::set_extrusion_color(lv_color_t color) {
    color_extrusion_ = color;
    use_custom_extrusion_color_ = true;
}

void GCodeLayerRenderer::set_travel_color(lv_color_t color) {
    color_travel_ = color;
    use_custom_travel_color_ = true;
}

void GCodeLayerRenderer::set_support_color(lv_color_t color) {
    color_support_ = color;
    use_custom_support_color_ = true;
}

void GCodeLayerRenderer::reset_colors() {
    // Use theme colors for default appearance
    // Extrusion: info blue for visibility against dark background
    color_extrusion_ = ui_theme_get_color("info_color");

    // Travel: subtle secondary color (grey)
    color_travel_ = ui_theme_get_color("text_secondary");

    // Support: orange/warning color to distinguish from model
    color_support_ = ui_theme_get_color("warning_color");

    use_custom_extrusion_color_ = false;
    use_custom_travel_color_ = false;
    use_custom_support_color_ = false;
}

// ============================================================================
// Viewport Control
// ============================================================================

void GCodeLayerRenderer::auto_fit() {
    if (!gcode_ || gcode_->layers.empty()) {
        scale_ = 1.0f;
        offset_x_ = 0.0f;
        offset_y_ = 0.0f;
        return;
    }

    // Use global bounding box for consistent framing
    const auto& bb = gcode_->global_bounding_box;

    float range_x, range_y;
    float center_x, center_y;

    switch (view_mode_) {
        case ViewMode::FRONT: {
            // Isometric-style: -45° horizontal + 30° elevation
            float xy_range_x = bb.max.x - bb.min.x;
            float xy_range_y = bb.max.y - bb.min.y;
            float z_range = bb.max.z - bb.min.z;

            // Horizontal extent after 45° rotation
            constexpr float COS_45 = 0.7071f;
            range_x = (xy_range_x + xy_range_y) * COS_45;

            // Vertical extent: Z * cos(30°) + Y_depth * sin(30°)
            constexpr float COS_30 = 0.866f;
            constexpr float SIN_30 = 0.5f;
            float y_depth = (xy_range_x + xy_range_y) * COS_45; // rotated Y range
            range_y = z_range * COS_30 + y_depth * SIN_30;

            center_x = (bb.min.x + bb.max.x) / 2.0f;
            center_y = (bb.min.y + bb.max.y) / 2.0f;
            offset_z_ = (bb.min.z + bb.max.z) / 2.0f;
            break;
        }

        case ViewMode::ISOMETRIC: {
            // Isometric: rotated X/Y view
            float xy_range_x = bb.max.x - bb.min.x;
            float xy_range_y = bb.max.y - bb.min.y;
            constexpr float ISO_ANGLE = 0.7071f;
            constexpr float ISO_Y_SCALE = 0.5f;
            range_x = (xy_range_x + xy_range_y) * ISO_ANGLE;
            range_y = (xy_range_x + xy_range_y) * ISO_ANGLE * ISO_Y_SCALE;
            center_x = (bb.min.x + bb.max.x) / 2.0f;
            center_y = (bb.min.y + bb.max.y) / 2.0f;
            break;
        }

        case ViewMode::TOP_DOWN:
        default:
            // Top-down: X/Y plane from above
            range_x = bb.max.x - bb.min.x;
            range_y = bb.max.y - bb.min.y;
            center_x = (bb.min.x + bb.max.x) / 2.0f;
            center_y = (bb.min.y + bb.max.y) / 2.0f;
            break;
    }

    // Handle degenerate cases
    if (range_x < 0.001f) range_x = 1.0f;
    if (range_y < 0.001f) range_y = 1.0f;

    // Add padding for visual breathing room
    constexpr float padding = 0.05f;
    range_x *= (1.0f + 2 * padding);
    range_y *= (1.0f + 2 * padding);

    // Scale to fit canvas (maintain aspect ratio)
    float scale_x = static_cast<float>(canvas_width_) / range_x;
    float scale_y = static_cast<float>(canvas_height_) / range_y;
    scale_ = std::min(scale_x, scale_y);

    // Store center for world_to_screen
    offset_x_ = center_x;
    offset_y_ = center_y;

    // Store bounds for reference (including Z for depth shading)
    bounds_min_x_ = bb.min.x;
    bounds_max_x_ = bb.max.x;
    bounds_min_y_ = bb.min.y;
    bounds_max_y_ = bb.max.y;
    bounds_min_z_ = bb.min.z;
    bounds_max_z_ = bb.max.z;

    bounds_valid_ = true;

    spdlog::debug("[GCodeLayerRenderer] auto_fit: mode={}, range=({:.1f},{:.1f}), scale={:.2f}",
                  static_cast<int>(view_mode_), range_x, range_y, scale_);
}

void GCodeLayerRenderer::fit_layer() {
    if (!gcode_ || gcode_->layers.empty()) {
        scale_ = 1.0f;
        offset_x_ = 0.0f;
        offset_y_ = 0.0f;
        return;
    }

    if (current_layer_ < 0 || current_layer_ >= static_cast<int>(gcode_->layers.size())) {
        return;
    }

    // Use current layer's bounding box
    const auto& bb = gcode_->layers[current_layer_].bounding_box;

    bounds_min_x_ = bb.min.x;
    bounds_max_x_ = bb.max.x;
    bounds_min_y_ = bb.min.y;
    bounds_max_y_ = bb.max.y;

    float range_x = bounds_max_x_ - bounds_min_x_;
    float range_y = bounds_max_y_ - bounds_min_y_;

    // Handle degenerate cases
    if (range_x < 0.001f) range_x = 1.0f;
    if (range_y < 0.001f) range_y = 1.0f;

    // Add padding
    constexpr float padding = 0.05f;
    range_x *= (1.0f + 2 * padding);
    range_y *= (1.0f + 2 * padding);

    // Scale to fit
    float scale_x = static_cast<float>(canvas_width_) / range_x;
    float scale_y = static_cast<float>(canvas_height_) / range_y;
    scale_ = std::min(scale_x, scale_y);

    // Center on layer
    offset_x_ = (bounds_min_x_ + bounds_max_x_) / 2.0f;
    offset_y_ = (bounds_min_y_ + bounds_max_y_) / 2.0f;

    bounds_valid_ = true;
}

void GCodeLayerRenderer::set_scale(float scale) {
    scale_ = std::max(0.001f, scale);
}

void GCodeLayerRenderer::set_offset(float x, float y) {
    offset_x_ = x;
    offset_y_ = y;
}

// ============================================================================
// Layer Information
// ============================================================================

GCodeLayerRenderer::LayerInfo GCodeLayerRenderer::get_layer_info() const {
    LayerInfo info{};
    info.layer_number = current_layer_;

    if (!gcode_ || gcode_->layers.empty()) {
        return info;
    }

    if (current_layer_ < 0 || current_layer_ >= static_cast<int>(gcode_->layers.size())) {
        return info;
    }

    const Layer& layer = gcode_->layers[current_layer_];
    info.z_height = layer.z_height;
    info.segment_count = layer.segments.size();
    info.extrusion_count = layer.segment_count_extrusion;
    info.travel_count = layer.segment_count_travel;

    // Check for support segments in this layer
    info.has_supports = false;
    for (const auto& seg : layer.segments) {
        if (is_support_segment(seg)) {
            info.has_supports = true;
            break;
        }
    }

    return info;
}

bool GCodeLayerRenderer::has_support_detection() const {
    // Support detection relies on object names from EXCLUDE_OBJECT
    // If there are named objects, we can potentially detect supports
    return gcode_ && !gcode_->objects.empty();
}

// ============================================================================
// Rendering
// ============================================================================

void GCodeLayerRenderer::destroy_cache() {
    if (cache_canvas_) {
        // Check if LVGL is still initialized and the object is valid
        // During shutdown, the widget tree may already be destroyed
        if (lv_is_initialized() && lv_obj_is_valid(cache_canvas_)) {
            lv_obj_delete(cache_canvas_);
        }
        cache_canvas_ = nullptr;
        // Note: canvas owns the draw_buf when attached, so don't double-free
        cache_buf_ = nullptr;
    } else if (cache_buf_) {
        // Buffer exists without canvas (shouldn't happen, but be safe)
        if (lv_is_initialized()) {
            lv_draw_buf_destroy(cache_buf_);
        }
        cache_buf_ = nullptr;
    }
    cached_up_to_layer_ = -1;
    cached_width_ = 0;
    cached_height_ = 0;
}

void GCodeLayerRenderer::invalidate_cache() {
    // Clear the cache buffer content but keep the canvas/buffer allocated
    if (cache_buf_) {
        lv_draw_buf_clear(cache_buf_, nullptr);
    }
    cached_up_to_layer_ = -1;

    // Also invalidate ghost cache (new gcode = need new ghost)
    if (ghost_buf_) {
        lv_draw_buf_clear(ghost_buf_, nullptr);
    }
    ghost_cache_valid_ = false;
    ghost_rendered_up_to_ = -1;
}

void GCodeLayerRenderer::ensure_cache(int width, int height) {
    // Recreate cache if dimensions changed
    if (cache_buf_ && (cached_width_ != width || cached_height_ != height)) {
        destroy_cache();
    }

    if (!cache_buf_) {
        // Create the draw buffer
        cache_buf_ = lv_draw_buf_create(width, height, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
        if (!cache_buf_) {
            spdlog::error("[GCodeLayerRenderer] Failed to create cache buffer {}x{}", width, height);
            return;
        }

        // Clear to transparent
        lv_draw_buf_clear(cache_buf_, nullptr);

        // Create a hidden canvas widget for offscreen rendering
        // We need a parent - use the top layer which always exists
        lv_obj_t* parent = lv_layer_top();
        if (!parent) {
            parent = lv_screen_active();
        }

        cache_canvas_ = lv_canvas_create(parent);
        if (!cache_canvas_) {
            spdlog::error("[GCodeLayerRenderer] Failed to create cache canvas");
            lv_draw_buf_destroy(cache_buf_);
            cache_buf_ = nullptr;
            return;
        }

        lv_canvas_set_draw_buf(cache_canvas_, cache_buf_);
        lv_obj_add_flag(cache_canvas_, LV_OBJ_FLAG_HIDDEN);  // Keep it invisible

        cached_width_ = width;
        cached_height_ = height;
        cached_up_to_layer_ = -1;

        spdlog::debug("[GCodeLayerRenderer] Created cache canvas: {}x{}", width, height);
    }
}

void GCodeLayerRenderer::render_layers_to_cache(int from_layer, int to_layer) {
    if (!cache_canvas_ || !cache_buf_ || !gcode_) return;

    // Initialize layer for drawing to the canvas (LVGL 9.4 canvas API)
    lv_layer_t cache_layer;
    lv_canvas_init_layer(cache_canvas_, &cache_layer);

    // Temporarily set widget offset to 0 since we're rendering to cache origin
    int saved_offset_x = widget_offset_x_;
    int saved_offset_y = widget_offset_y_;
    widget_offset_x_ = 0;
    widget_offset_y_ = 0;

    size_t segments_rendered = 0;
    for (int layer_idx = from_layer; layer_idx <= to_layer; ++layer_idx) {
        if (layer_idx < 0 || layer_idx >= static_cast<int>(gcode_->layers.size())) continue;

        const Layer& layer_data = gcode_->layers[layer_idx];
        for (const auto& seg : layer_data.segments) {
            if (should_render_segment(seg)) {
                render_segment(&cache_layer, seg);
                ++segments_rendered;
            }
        }
    }

    // Finish the layer - flushes all pending draw tasks to the buffer
    lv_canvas_finish_layer(cache_canvas_, &cache_layer);

    widget_offset_x_ = saved_offset_x;
    widget_offset_y_ = saved_offset_y;

    spdlog::debug("[GCodeLayerRenderer] Rendered layers {}-{}: {} segments to cache",
                  from_layer, to_layer, segments_rendered);
}

void GCodeLayerRenderer::blit_cache(lv_layer_t* target) {
    if (!cache_buf_) return;

    lv_draw_image_dsc_t dsc;
    lv_draw_image_dsc_init(&dsc);
    dsc.src = cache_buf_;

    lv_area_t coords = {widget_offset_x_, widget_offset_y_,
                        widget_offset_x_ + cached_width_ - 1,
                        widget_offset_y_ + cached_height_ - 1};

    lv_draw_image(target, &dsc, &coords);
}

// ============================================================================
// Ghost Cache (faded preview of all layers)
// ============================================================================

void GCodeLayerRenderer::destroy_ghost_cache() {
    if (ghost_canvas_) {
        if (lv_is_initialized() && lv_obj_is_valid(ghost_canvas_)) {
            lv_obj_delete(ghost_canvas_);
        }
        ghost_canvas_ = nullptr;
        ghost_buf_ = nullptr;
    } else if (ghost_buf_) {
        if (lv_is_initialized()) {
            lv_draw_buf_destroy(ghost_buf_);
        }
        ghost_buf_ = nullptr;
    }
    ghost_cache_valid_ = false;
    ghost_rendered_up_to_ = -1;
}

void GCodeLayerRenderer::ensure_ghost_cache(int width, int height) {
    // Recreate if dimensions changed
    if (ghost_buf_ && (cached_width_ != width || cached_height_ != height)) {
        destroy_ghost_cache();
    }

    if (!ghost_buf_) {
        ghost_buf_ = lv_draw_buf_create(width, height, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
        if (!ghost_buf_) {
            spdlog::error("[GCodeLayerRenderer] Failed to create ghost buffer {}x{}", width, height);
            return;
        }

        lv_draw_buf_clear(ghost_buf_, nullptr);

        lv_obj_t* parent = lv_layer_top();
        if (!parent) parent = lv_screen_active();

        ghost_canvas_ = lv_canvas_create(parent);
        if (!ghost_canvas_) {
            lv_draw_buf_destroy(ghost_buf_);
            ghost_buf_ = nullptr;
            return;
        }

        lv_canvas_set_draw_buf(ghost_canvas_, ghost_buf_);
        lv_obj_add_flag(ghost_canvas_, LV_OBJ_FLAG_HIDDEN);

        ghost_cache_valid_ = false;
        spdlog::debug("[GCodeLayerRenderer] Created ghost cache canvas: {}x{}", width, height);
    }
}

void GCodeLayerRenderer::render_ghost_layers(int from_layer, int to_layer) {
    if (!ghost_canvas_ || !ghost_buf_ || !gcode_) return;

    lv_layer_t ghost_layer;
    lv_canvas_init_layer(ghost_canvas_, &ghost_layer);

    int saved_offset_x = widget_offset_x_;
    int saved_offset_y = widget_offset_y_;
    widget_offset_x_ = 0;
    widget_offset_y_ = 0;

    size_t segments_rendered = 0;
    for (int layer_idx = from_layer; layer_idx <= to_layer; ++layer_idx) {
        if (layer_idx < 0 || layer_idx >= static_cast<int>(gcode_->layers.size())) continue;

        const Layer& layer_data = gcode_->layers[layer_idx];
        for (const auto& seg : layer_data.segments) {
            if (should_render_segment(seg)) {
                // Render with reduced opacity for ghost effect
                render_segment(&ghost_layer, seg, true);  // ghost=true
                ++segments_rendered;
            }
        }
    }

    lv_canvas_finish_layer(ghost_canvas_, &ghost_layer);

    widget_offset_x_ = saved_offset_x;
    widget_offset_y_ = saved_offset_y;

    spdlog::debug("[GCodeLayerRenderer] Rendered ghost layers {}-{}: {} segments",
                  from_layer, to_layer, segments_rendered);
}

void GCodeLayerRenderer::blit_ghost_cache(lv_layer_t* target) {
    if (!ghost_buf_) return;

    lv_draw_image_dsc_t dsc;
    lv_draw_image_dsc_init(&dsc);
    dsc.src = ghost_buf_;
    dsc.opa = LV_OPA_40;  // 40% opacity for ghost

    lv_area_t coords = {widget_offset_x_, widget_offset_y_,
                        widget_offset_x_ + cached_width_ - 1,
                        widget_offset_y_ + cached_height_ - 1};

    lv_draw_image(target, &dsc, &coords);
}

void GCodeLayerRenderer::render(lv_layer_t* layer, const lv_area_t* widget_area) {
    if (!gcode_ || gcode_->layers.empty()) {
        spdlog::debug("[GCodeLayerRenderer] render(): no gcode data");
        return;
    }

    if (current_layer_ < 0 || current_layer_ >= static_cast<int>(gcode_->layers.size())) {
        spdlog::debug("[GCodeLayerRenderer] render(): layer out of range ({} / {})",
                      current_layer_, gcode_->layers.size());
        return;
    }

    uint32_t start_time = lv_tick_get();

    // Store widget screen offset for world_to_screen()
    if (widget_area) {
        widget_offset_x_ = widget_area->x1;
        widget_offset_y_ = widget_area->y1;
    }

    // Auto-fit if bounds not yet computed
    if (!bounds_valid_) {
        auto_fit();
    }

    size_t segments_rendered = 0;

    // For FRONT view, use incremental cache with progressive rendering
    if (view_mode_ == ViewMode::FRONT) {
        int target_layer = std::min(current_layer_, static_cast<int>(gcode_->layers.size()) - 1);
        int max_layer = static_cast<int>(gcode_->layers.size()) - 1;

        // Ensure cache buffers exist and are correct size
        ensure_cache(canvas_width_, canvas_height_);
        if (ghost_mode_enabled_) {
            ensure_ghost_cache(canvas_width_, canvas_height_);
        }

        // =====================================================================
        // GHOST CACHE: Progressive rendering of ALL layers (one-time)
        // Renders GHOST_LAYERS_PER_FRAME each draw cycle until complete
        // =====================================================================
        if (ghost_mode_enabled_ && ghost_buf_ && ghost_canvas_ && !ghost_cache_valid_) {
            if (ghost_rendered_up_to_ >= max_layer) {
                ghost_cache_valid_ = true;
            } else {
                int from_layer = ghost_rendered_up_to_ + 1;
                int to_layer = std::min(from_layer + GHOST_LAYERS_PER_FRAME - 1, max_layer);

                render_ghost_layers(from_layer, to_layer);
                ghost_rendered_up_to_ = to_layer;

                if (ghost_rendered_up_to_ >= max_layer) {
                    ghost_cache_valid_ = true;
                    spdlog::info("[GCodeLayerRenderer] Ghost cache complete: {} layers", max_layer + 1);
                }
            }
        }

        // =====================================================================
        // SOLID CACHE: Progressive rendering up to current print layer
        // =====================================================================
        if (cache_buf_ && cache_canvas_) {
            // Check if we need to render new layers
            if (target_layer > cached_up_to_layer_) {
                // Progressive rendering: only render up to LAYERS_PER_FRAME at a time
                // This prevents UI freezing during initial load or big jumps
                int from_layer = cached_up_to_layer_ + 1;
                int to_layer = std::min(from_layer + LAYERS_PER_FRAME - 1, target_layer);

                render_layers_to_cache(from_layer, to_layer);
                cached_up_to_layer_ = to_layer;

                // If we haven't caught up yet, caller should check needs_more_frames()
                // and invalidate the widget to trigger another frame
                if (cached_up_to_layer_ < target_layer) {
                    spdlog::debug("[GCodeLayerRenderer] Progressive: rendered to layer {}/{}, more needed",
                                  cached_up_to_layer_, target_layer);
                }
            } else if (target_layer < cached_up_to_layer_) {
                // Going backwards - need to re-render from scratch (progressively)
                lv_draw_buf_clear(cache_buf_, nullptr);
                cached_up_to_layer_ = -1;

                int to_layer = std::min(LAYERS_PER_FRAME - 1, target_layer);
                render_layers_to_cache(0, to_layer);
                cached_up_to_layer_ = to_layer;
                // Caller checks needs_more_frames() for continuation
            }
            // else: same layer, just blit cached image

            // =====================================================================
            // BLIT: Ghost first (underneath), then solid on top
            // =====================================================================
            if (ghost_mode_enabled_ && ghost_buf_) {
                blit_ghost_cache(layer);
            }
            blit_cache(layer);
            segments_rendered = last_segment_count_;
        }
    } else {
        // TOP_DOWN or ISOMETRIC: render single layer directly (no caching needed)
        const auto& layer_bb = gcode_->layers[current_layer_].bounding_box;
        offset_x_ = (layer_bb.min.x + layer_bb.max.x) / 2.0f;
        offset_y_ = (layer_bb.min.y + layer_bb.max.y) / 2.0f;

        const Layer& current = gcode_->layers[current_layer_];
        for (const auto& seg : current.segments) {
            if (!should_render_segment(seg)) continue;
            render_segment(layer, seg);
            ++segments_rendered;
        }
    }

    // Track render time for diagnostics
    last_render_time_ms_ = lv_tick_get() - start_time;
    last_segment_count_ = segments_rendered;

    // Log performance if layer changed or slow render
    if (current_layer_ != last_rendered_layer_ || last_render_time_ms_ > 50) {
        spdlog::debug("[GCodeLayerRenderer] Layer {}: {}ms (cached_up_to={})",
                      current_layer_, last_render_time_ms_, cached_up_to_layer_);
        last_rendered_layer_ = current_layer_;
    }
}

bool GCodeLayerRenderer::needs_more_frames() const {
    if (!gcode_ || gcode_->layers.empty()) {
        return false;
    }

    // Only relevant for FRONT view mode (uses caching)
    if (view_mode_ != ViewMode::FRONT) {
        return false;
    }

    int target_layer = std::min(current_layer_, static_cast<int>(gcode_->layers.size()) - 1);
    int max_layer = static_cast<int>(gcode_->layers.size()) - 1;

    // Solid cache incomplete?
    if (cached_up_to_layer_ < target_layer) {
        return true;
    }

    // Ghost cache incomplete?
    if (ghost_mode_enabled_ && !ghost_cache_valid_ && ghost_rendered_up_to_ < max_layer) {
        return true;
    }

    return false;
}

bool GCodeLayerRenderer::should_render_segment(const ToolpathSegment& seg) const {
    if (seg.is_extrusion) {
        if (is_support_segment(seg)) {
            return show_supports_;
        }
        return show_extrusions_;
    }
    return show_travels_;
}

void GCodeLayerRenderer::render_segment(lv_layer_t* layer, const ToolpathSegment& seg, bool ghost) {
    // Convert world coordinates to screen (uses Z for FRONT view)
    glm::ivec2 p1 = world_to_screen(seg.start.x, seg.start.y, seg.start.z);
    glm::ivec2 p2 = world_to_screen(seg.end.x, seg.end.y, seg.end.z);

    // Skip zero-length segments
    if (p1.x == p2.x && p1.y == p2.y) {
        return;
    }

    // Initialize line drawing descriptor
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);

    lv_color_t base_color;
    if (ghost) {
        // Ghost mode: use darkened version of the model's extrusion color
        // This provides visual continuity between ghost and solid layers
        lv_color_t model_color = color_extrusion_;
        // Darken to 40% brightness for ghost effect
        base_color = lv_color_make(model_color.red * 40 / 100, model_color.green * 40 / 100,
                                   model_color.blue * 40 / 100);
    } else {
        base_color = get_segment_color(seg);
    }

    // Apply depth shading for 3D-like appearance
    if (depth_shading_ && view_mode_ == ViewMode::FRONT) {
        // Calculate brightness factor based on Z position
        // Bottom of model = darker (40%), top = brighter (100%)
        float z_range = bounds_max_z_ - bounds_min_z_;
        float avg_z = (seg.start.z + seg.end.z) / 2.0f;

        float brightness = 0.4f; // Minimum brightness
        if (z_range > 0.001f) {
            float normalized_z = (avg_z - bounds_min_z_) / z_range;
            brightness = 0.4f + 0.6f * normalized_z; // 40% to 100%
        }

        // Also add subtle Y-depth fade (back of model slightly darker)
        // In 45° view, higher Y values are further back
        float y_range = bounds_max_y_ - bounds_min_y_;
        float avg_y = (seg.start.y + seg.end.y) / 2.0f;
        if (y_range > 0.001f) {
            float normalized_y = (avg_y - bounds_min_y_) / y_range;
            // Front (low Y) = 100%, back (high Y) = 85%
            float depth_fade = 0.85f + 0.15f * (1.0f - normalized_y);
            brightness *= depth_fade;
        }

        // Apply brightness to color (scale RGB channels)
        // LVGL 9: lv_color_t has direct .red, .green, .blue members
        uint8_t r = static_cast<uint8_t>(base_color.red * brightness);
        uint8_t g = static_cast<uint8_t>(base_color.green * brightness);
        uint8_t b = static_cast<uint8_t>(base_color.blue * brightness);
        dsc.color = lv_color_make(r, g, b);
    } else {
        dsc.color = base_color;
    }

    // Extrusion moves: thicker, fully opaque
    // Travel moves: thinner, semi-transparent
    if (seg.is_extrusion) {
        dsc.width = 2;
        dsc.opa = LV_OPA_COVER;
    } else {
        dsc.width = 1;
        dsc.opa = LV_OPA_50;
    }

    // LVGL 9: points are stored in the descriptor struct
    dsc.p1.x = static_cast<lv_value_precise_t>(p1.x);
    dsc.p1.y = static_cast<lv_value_precise_t>(p1.y);
    dsc.p2.x = static_cast<lv_value_precise_t>(p2.x);
    dsc.p2.y = static_cast<lv_value_precise_t>(p2.y);

    lv_draw_line(layer, &dsc);
}

glm::ivec2 GCodeLayerRenderer::world_to_screen(float x, float y, float z) const {
    float sx, sy;

    switch (view_mode_) {
        case ViewMode::FRONT: {
            // Isometric-style view: 45° horizontal rotation + 30° elevation
            // This creates a "corner view looking down" perspective
            //
            // First apply 90° CCW rotation around Z to match thumbnail orientation
            // (thumbnails show models from a different default angle)
            float raw_dx = x - offset_x_;
            float raw_dy = y - offset_y_;
            float dx = -raw_dy;  // 90° CCW: new_x = -old_y
            float dy = raw_dx;   // 90° CCW: new_y = old_x
            float dz = z - offset_z_;

            // Horizontal rotation: -45° (negative = view from front-right corner)
            // sin(-45°) = -0.7071, cos(-45°) = 0.7071
            constexpr float COS_H = 0.7071f;  // cos(45°)
            constexpr float SIN_H = -0.7071f; // sin(-45°) - negative for other corner

            // Elevation angle: 30° looking down
            // sin(30°) = 0.5, cos(30°) = 0.866
            constexpr float COS_E = 0.866f;  // cos(30°)
            constexpr float SIN_E = 0.5f;    // sin(30°)

            // Apply horizontal rotation first (around Z axis)
            float rx = dx * COS_H - dy * SIN_H;
            float ry = dx * SIN_H + dy * COS_H;

            // Then apply elevation (tilt camera down)
            // Screen X = rotated X
            // Screen Y = -Z * cos(elev) + rotated_Y * sin(elev)
            sx = rx * scale_ + static_cast<float>(canvas_width_) / 2.0f;
            sy = static_cast<float>(canvas_height_) / 2.0f - (dz * COS_E + ry * SIN_E) * scale_;
            break;
        }

        case ViewMode::ISOMETRIC: {
            // Isometric projection (45° rotation with Y compression)
            float dx = x - offset_x_;
            float dy = y - offset_y_;
            constexpr float ISO_ANGLE = 0.7071f;
            constexpr float ISO_Y_SCALE = 0.5f;

            float iso_x = (dx - dy) * ISO_ANGLE;
            float iso_y = (dx + dy) * ISO_ANGLE * ISO_Y_SCALE;

            sx = iso_x * scale_ + static_cast<float>(canvas_width_) / 2.0f;
            sy = static_cast<float>(canvas_height_) / 2.0f - iso_y * scale_;
            break;
        }

        case ViewMode::TOP_DOWN:
        default: {
            // Top-down: X → screen X, Y → screen Y (flipped)
            float dx = x - offset_x_;
            float dy = y - offset_y_;
            sx = dx * scale_ + static_cast<float>(canvas_width_) / 2.0f;
            sy = static_cast<float>(canvas_height_) / 2.0f - dy * scale_;
            break;
        }
    }

    // Add widget's screen offset (drawing is in screen coordinates)
    return {static_cast<int>(sx) + widget_offset_x_, static_cast<int>(sy) + widget_offset_y_};
}

bool GCodeLayerRenderer::is_support_segment(const ToolpathSegment& seg) const {
    // Support detection via object name (from EXCLUDE_OBJECT metadata)
    if (seg.object_name.empty()) {
        return false;
    }

    // Common patterns used by slicers for support structures:
    // - OrcaSlicer/PrusaSlicer: "support_*", "*_support", "SUPPORT_*"
    // - Cura: "support", "Support"
    const std::string& name = seg.object_name;

    // Case-insensitive check for "support" anywhere in the name
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    return lower_name.find("support") != std::string::npos;
}

lv_color_t GCodeLayerRenderer::get_segment_color(const ToolpathSegment& seg) const {
    if (!seg.is_extrusion) {
        // Travel move
        return color_travel_;
    }

    // Check if this is a support segment
    if (is_support_segment(seg)) {
        return color_support_;
    }

    // Regular extrusion
    return color_extrusion_;
}

} // namespace gcode
} // namespace helix
