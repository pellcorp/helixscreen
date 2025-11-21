# Session Handoff Document

**Last Updated:** 2025-11-20
**Current Focus:** G-code Viewer Polish & Bed Mesh Enhancements

---

## ✅ CURRENT STATE

### Recently Completed

**G-Code Viewer Command-Line Options (2025-11-20)**
- ✅ Added `--gcode-file <path>` to load specific G-code file on startup
- ✅ Added `--gcode-az <deg>` to set camera azimuth angle
- ✅ Added `--gcode-el <deg>` to set camera elevation angle
- ✅ Added `--gcode-zoom <n>` to set camera zoom level
- ✅ Added `--gcode-debug-colors` to enable per-face debug coloring
- ✅ Exposed camera setters (set_azimuth/set_elevation/set_zoom_level) in GCodeCamera
- ✅ Added ui_gcode_viewer API functions for camera control and debug colors
- ✅ Settings applied automatically in gcode test panel on startup

**Files Modified:**
- `include/runtime_config.h` - Added G-code viewer config fields
- `src/main.cpp` - Added command-line argument parsing
- `include/gcode_camera.h` - Added direct setter methods
- `src/gcode_camera.cpp` - Implemented camera setters
- `include/ui_gcode_viewer.h` - Added viewer widget API functions
- `src/ui_gcode_viewer.cpp` - Implemented widget wrapper functions
- `src/ui_panel_gcode_test.cpp` - Apply runtime config on panel init

**Usage Examples:**
```bash
# Custom camera angles
./build/bin/helix-ui-proto -p gcode-test --gcode-az 90 --gcode-el -10 --gcode-zoom 5

# Load specific file with debug colors
./build/bin/helix-ui-proto -p gcode-test --gcode-file assets/test_gcode/multi_color_cube.gcode --gcode-debug-colors

# All options combined
./build/bin/helix-ui-proto -p gcode-test --test \
  --gcode-file my_print.gcode \
  --gcode-az 45 --gcode-el 30 --gcode-zoom 2.5 \
  --gcode-debug-colors
```

**G-Code 3D Rendering - Geometry Fixes (Earlier 2025-11-20)**
- ✅ Fixed tube geometry cross-section calculation (perpendicular vectors)
- ✅ Corrected end cap rendering (proper vertex ordering)
- ✅ Fixed tube proportions using `layer_height_mm` from G-code metadata
- ✅ Corrected face normals to point outward (top=UP, bottom=DOWN)
- ✅ Added per-face debug coloring system for geometry diagnosis
- ✅ Added camera debug overlay showing Az/El/Zoom (only with -vv flag)

**Commits:**
- (Pending) - feat(gcode): add command-line options for camera and debug controls
- `d02b140` - fix(gcode): correct tube geometry cross-section and end cap rendering
- `f56edde` - Add single extrusion test G-code file

### Recently Completed (Previous Sessions)

**Bed Mesh Grid Lines Implementation:**
- ✅ Wireframe grid overlay on mesh surface
- ✅ Dark gray (80,80,80) lines with 60% opacity
- ✅ LVGL 9.4 layer-based drawing (lv_canvas_init_layer/finish_layer)
- ✅ Coordinate system matches mesh quads exactly (Y-inversion, Z-centering)
- ✅ Horizontal and vertical lines connecting mesh vertices
- ✅ Bounds checking with -10px margin for partially visible lines
- ✅ Defensive checks for invalid canvas dimensions during flex layout

**Commits:**
- `dc2742e` - feat(bed_mesh): implement wireframe grid lines over mesh surface
- `e97b141` - refactor(bed_mesh): remove frontend fallback, use theme constants
- `e196b48` - fix(bed_mesh): dynamic canvas buffer and manual label updates
- `6ebf122` - fix(bed_mesh): reactive bindings now working with nullptr prev buffer

---

## 🚀 NEXT PRIORITIES

### 1. **G-Code Viewer - Layer Controls** (HIGH PRIORITY)

**Remaining Tasks:**
- [ ] Add layer slider/scrubber for preview-by-layer
- [ ] Add layer range selector (start/end layer)
- [ ] Show current layer number in UI
- [ ] Test with complex multi-color G-code files

**Command-Line Testing:**
```bash
# Test with custom camera angles
./build/bin/helix-ui-proto -p gcode-test --test --gcode-az 90 --gcode-el -10 --gcode-zoom 5

# Test with multi-color file and debug colors
./build/bin/helix-ui-proto -p gcode-test --test \
  --gcode-file assets/test_gcode/multi_color_cube.gcode \
  --gcode-debug-colors

# Test specific camera position for screenshots
./build/bin/helix-ui-proto -p gcode-test --test \
  --gcode-file my_print.gcode \
  --gcode-az 45 --gcode-el 30 --gcode-zoom 2.0 \
  --screenshot 2
```

### 2. **Complete Bed Mesh UI Features** (MEDIUM PRIORITY)

**Goal:** Match feature parity with GuppyScreen bed mesh visualization

**Completed:**
- ✅ Rotation sliders (Tilt/Spin) - visible and functional
- ✅ Info labels (dimensions, Z range) - visible with reactive bindings
- ✅ Grid lines - wireframe overlay properly aligned

**Remaining Tasks:**
1. [ ] Add axis labels (X/Y/Z indicators, bed dimensions)
2. [ ] Add mesh profile selector dropdown
3. [ ] Display additional statistics (variance/deviation)

### 3. **Print Select Integration**

**Goal:** Add "Preview" button to print file browser

**Tasks:**
- [ ] Add preview button to file list items in print_select_panel.xml
- [ ] Fetch G-code via Moonraker HTTP API
- [ ] Open viewer in overlay panel
- [ ] Show filename and stats (layers, print time)

---

## 📋 Critical Patterns Reference

### Pattern #0: Per-Face Debug Coloring (NEW)

**Purpose:** Diagnose 3D geometry issues by coloring each face differently

**Usage:**
```cpp
// Enable in renderer constructor or via method
geometry_builder_->set_debug_face_colors(true);

// Colors assigned:
// - Top face: Red (#FF0000)
// - Bottom face: Blue (#0000FF)
// - Left face: Green (#00FF00)
// - Right face: Yellow (#FFFF00)
// - Start cap: Magenta (#FF00FF)
// - End cap: Cyan (#00FFFF)
```

**Implementation:**
- Creates 6 color palette entries when enabled
- Assigns colors based on vertex face membership
- Vertex order: [0-1]=bottom, [2-3]=right, [4-5]=top, [6-7]=left
- Logs once per build: "DEBUG FACE COLORS ACTIVE: ..."

**When to Use:**
- Diagnosing twisted/kinked geometry
- Verifying face orientation and winding order
- Checking vertex ordering correctness
- Validating normal calculations

### Pattern #1: G-Code Camera Angles

**Exact test angle for geometry verification:**
```cpp
// In gcode_camera.cpp reset():
azimuth_ = 85.5f;    // Horizontal rotation
elevation_ = -2.5f;  // Slight downward tilt
zoom_level_ = 10.0f; // 10x zoom (preserved by fit_to_bounds)
```

**Debug Overlay (requires -vv flag):**
- Shows "Az: 85.5° El: -2.5° Zoom: 10.0x" in upper-left
- Only visible with debug logging enabled
- Helps reproduce exact viewing conditions

### Pattern #2: LV_SIZE_CONTENT Bug

**NEVER use `height="LV_SIZE_CONTENT"` or `height="auto"` with complex nested children in flex layouts.**

```xml
<!-- ❌ WRONG - collapses to 0 height -->
<ui_card height="LV_SIZE_CONTENT" flex_flow="column">
  <text_heading>...</text_heading>
  <lv_dropdown>...</lv_dropdown>
</ui_card>

<!-- ✅ CORRECT - uses flex grow -->
<ui_card flex_grow="1" flex_flow="column">
  <text_heading>...</text_heading>
  <lv_dropdown>...</lv_dropdown>
</ui_card>
```

### Pattern #3: Bed Mesh Widget API

**Custom LVGL widget for bed mesh canvas:**

```cpp
#include "ui_bed_mesh.h"

// Set mesh data (triggers automatic redraw)
std::vector<const float*> row_pointers;
// ... populate row_pointers ...
ui_bed_mesh_set_data(canvas, row_pointers.data(), rows, cols);

// Update rotation (triggers automatic redraw)
ui_bed_mesh_set_rotation(canvas, tilt_angle, spin_angle);

// Force redraw
ui_bed_mesh_redraw(canvas);
```

**Widget automatically manages:**
- Canvas buffer allocation (600×400 RGB888 = 720KB)
- Renderer lifecycle (create on init, destroy on delete)
- Layout updates (calls lv_obj_update_layout before render)
- Bounds checking (clips all coordinates to canvas)

### Pattern #4: Reactive Subjects for Mesh Data

```cpp
// Initialize subjects
static lv_subject_t bed_mesh_dimensions;
static char dimensions_buf[64] = "No mesh data";
lv_subject_init_string(&bed_mesh_dimensions, dimensions_buf,
                       prev_buf, sizeof(dimensions_buf), "No mesh data");

// Update when mesh changes
snprintf(dimensions_buf, sizeof(dimensions_buf), "%dx%d points", rows, cols);
lv_subject_copy_string(&bed_mesh_dimensions, dimensions_buf);
```

```xml
<!-- Bind label to subject -->
<lv_label name="mesh_dimensions_label" bind_text="bed_mesh_dimensions"/>
```

### Pattern #5: Thread Management - NEVER Block UI Thread

**CRITICAL:** NEVER use blocking operations like `thread.join()` in code paths triggered by UI events.

```cpp
// ❌ WRONG - Blocks LVGL main thread
if (connect_thread_.joinable()) {
    connect_thread_.join();  // UI FREEZES HERE
}

// ✅ CORRECT - Non-blocking cleanup
connect_active_ = false;
if (connect_thread_.joinable()) {
    connect_thread_.detach();
}
```

### Pattern #6: G-code Viewer Widget API

**Custom LVGL widget for G-code 3D visualization:**

```cpp
#include "ui_gcode_viewer.h"

// Create viewer widget
lv_obj_t* viewer = ui_gcode_viewer_create(parent);

// Load G-code file
ui_gcode_viewer_load_file(viewer, "/path/to/file.gcode");

// Change view
ui_gcode_viewer_set_view(viewer, GCODE_VIEW_ISOMETRIC);
ui_gcode_viewer_set_view(viewer, GCODE_VIEW_TOP);

// Reset camera
ui_gcode_viewer_reset_view(viewer);
```

```xml
<!-- Use in XML -->
<gcode_viewer name="my_viewer" width="100%" height="400"/>
```

**Widget features:**
- Touch drag to rotate camera
- Automatic fit-to-bounds framing
- Preset view buttons
- State management (EMPTY/LOADING/LOADED/ERROR)

---

## 📚 Key Documentation

- **G-code Visualization:** `docs/GCODE_VISUALIZATION.md` - Complete system design and integration guide
- **Bed Mesh Analysis:** `docs/GUPPYSCREEN_BEDMESH_ANALYSIS.md` - GuppyScreen renderer analysis
- **Implementation Patterns:** `docs/BEDMESH_IMPLEMENTATION_PATTERNS.md` - Code templates
- **Renderer API:** `docs/BEDMESH_RENDERER_INDEX.md` - bed_mesh_renderer.h reference
- **Widget APIs:** `include/ui_bed_mesh.h`, `include/ui_gcode_viewer.h` - Custom widget public APIs

---

## 🐛 Known Issues

1. **Missing Bed Mesh UI Features**
   - ✅ Grid lines - IMPLEMENTED
   - ✅ Info labels - IMPLEMENTED
   - ✅ Rotation sliders - IMPLEMENTED
   - ❌ Axis labels not implemented
   - ❌ Mesh profile selector not implemented
   - ❌ Variance/deviation statistics not displayed

2. **No Bed Mesh Profile Switching**
   - Can fetch multiple profiles from Moonraker
   - No UI to switch between profiles

3. **G-code Viewer Not Integrated**
   - Standalone test panel works (`-p gcode-test`)
   - Not yet integrated with print select panel
   - No "Preview" button in file browser

---

## 🔍 Debugging Tips

**G-Code Viewer Testing:**
```bash
# Run with debug logging and camera overlay
./build/bin/helix-ui-proto -p gcode-test -vv --test

# Test multi-color files
./build/bin/helix-ui-proto -p gcode-test --test-file assets/test_gcode/multi_color_cube.gcode

# Camera overlay shows (upper-left, only with -vv):
# "Az: 85.5° El: -2.5° Zoom: 10.0x"
```

**Screenshot Current State:**
```bash
# Take screenshot
./scripts/screenshot.sh helix-ui-proto gcode-test gcode-test

# View screenshot
open /tmp/ui-screenshot-gcode-test.png
```

**Per-Face Debug Coloring (if needed):**
```cpp
// Enable in gcode_tinygl_renderer.cpp:31
geometry_builder_->set_debug_face_colors(true);

// Colors: Top=Red, Bottom=Blue, Left=Green, Right=Yellow,
//         StartCap=Magenta, EndCap=Cyan
```
