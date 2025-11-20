# Multi-Color G-Code Implementation Plan

**Status:** Ready to implement
**Estimated Effort:** 5-8 hours
**Files to Modify:** 3 core files
**Test File:** `assets/OrcaCube_ABS_Multicolor.gcode`

---

## Executive Summary

The G-code viewer currently renders all objects in a single color (or Z-height gradient). Multi-color prints from tools like OrcaSlicer display incorrectly - all segments appear the same color instead of showing distinct colors for each extruder/tool.

**Root Cause:** The system was architecturally designed for single-color prints:
1. Parser ignores tool change commands (`T0`, `T1`, `T2`)
2. `ToolpathSegment` struct has no field to store which tool was active
3. Colors are computed algorithmically (Z-gradient OR single solid color) - never from actual tool data

**Solution:** Parse standard `Tx` commands + header color metadata, tag segments with tool indices, and lookup colors from palette during rendering.

---

## Problem Analysis

### Current Color Computation (`gcode_geometry_builder.cpp:390-392`)

```cpp
// Compute colors based on Z-height (rainbow gradient)
float mid_z = (segment.start.z + segment.end.z) * 0.5f;
uint32_t rgb = compute_color_rgb(mid_z, quant.min_bounds.z, quant.max_bounds.z);
```

The `compute_color_rgb()` function (lines 595-658) has only TWO modes:
- **Height gradient:** Rainbow spectrum blue → red based on Z
- **Single filament color:** One global color for ALL segments

**No mechanism exists to vary color per segment based on tool/extruder.**

### Example File Analysis (`assets/OrcaCube_ABS_Multicolor.gcode`)

Contains:
- **51 tool changes:** `T0` (red) and `T2` (beige)
- **Color metadata:** `; extruder_colour = #ED1C24;#00C1AE;#F4E2C1;#000000`
- **50 wipe tower sections:** Marked with `; WIPE_TOWER_START` / `; WIPE_TOWER_END`
- **2 print objects:** `OrcaCube_id_0_copy_0` and `OrcaPlug_id_1_copy_0`

**All tool changes and color metadata are currently ignored.**

---

## Solution Design

### Approach: Tx Commands + Header Metadata (100% Portable)

**Why this works:**
- Standard `T0`, `T1`, `T2` commands exist in ALL G-code (Marlin, Klipper, RepRap)
- Header metadata `extruder_colour` provides color palette
- No dependency on custom macros (TOOLCHANGE is Klipper-specific)
- Simple state machine: track one integer (current tool index)

**Data flow:**
```
Header parsing → Build color palette array
  ↓
G-code parsing → Detect Tx commands → Update current_tool_index
  ↓
Segment creation → Tag segment.tool_index = current_tool_index
  ↓
Geometry building → Lookup palette[segment.tool_index] → Convert hex to RGB
```

---

## Implementation Plan

### Phase 1: Parser Enhancement (REQUIRED)

**Files:** `include/gcode_parser.h`, `src/gcode_parser.cpp`

#### 1.1 Add Tool State to Parser Class (`gcode_parser.h`)

```cpp
class GCodeParser {
private:
    // NEW: Tool tracking
    int current_tool_index_ = 0;  // Active extruder (0-indexed)
    std::vector<std::string> tool_color_palette_;  // Hex colors: ["#ED1C24", "#00C1AE", ...]
    bool in_wipe_tower_ = false;  // For optional wipe tower exclusion

    // NEW: Helper methods
    void parse_extruder_color_metadata(const std::string& line);
    void parse_tool_change_command(const std::string& line);
    void parse_wipe_tower_marker(const std::string& comment);
};
```

#### 1.2 Modify ToolpathSegment Struct (`gcode_parser.h:92-99`)

```cpp
struct ToolpathSegment {
    glm::vec3 start;           ///< Starting position (mm)
    glm::vec3 end;             ///< Ending position (mm)
    bool is_extrusion;         ///< True if extruding (E > 0)
    std::string object_name;   ///< Object name or empty
    float extrusion_amount;    ///< E axis delta for this segment

    // NEW: Multi-color support
    int tool_index = 0;        ///< Which tool/extruder printed this (0-indexed)
};
```

#### 1.3 Parse Header Metadata (`gcode_parser.cpp:248-406`)

Add to `parse_metadata_comment()` or create new helper:

```cpp
void GCodeParser::parse_extruder_color_metadata(const std::string& line) {
    // Format: "; extruder_colour = #ED1C24;#00C1AE;#F4E2C1;#000000"
    //     OR: "; filament_colour = ..." (fallback)

    size_t pos = line.find(" = ");
    if (pos == std::string::npos) return;

    std::string colors_str = line.substr(pos + 3);

    // Split by semicolons
    std::stringstream ss(colors_str);
    std::string color;
    while (std::getline(ss, color, ';')) {
        // Trim whitespace
        color.erase(0, color.find_first_not_of(" \t\r\n"));
        color.erase(color.find_last_not_of(" \t\r\n") + 1);

        if (!color.empty() && color[0] == '#') {
            tool_color_palette_.push_back(color);
        } else {
            // Empty entry or invalid format
            tool_color_palette_.push_back("");  // Placeholder
        }
    }

    spdlog::debug("Parsed {} extruder colors", tool_color_palette_.size());
}
```

Call from existing metadata parsing (around line 300-350):

```cpp
if (comment.find("extruder_colour") != std::string::npos) {
    parse_extruder_color_metadata(line);
}
// Fallback to filament_colour if extruder_colour not found
else if (comment.find("filament_colour") != std::string::npos &&
         tool_color_palette_.empty()) {
    parse_extruder_color_metadata(line);
}
```

#### 1.4 Parse Tool Change Commands (`gcode_parser.cpp:83-124`)

Add to main parsing loop before movement commands:

```cpp
void GCodeParser::parse_tool_change_command(const std::string& line) {
    // Format: "T0", "T1", "T2", etc. (standalone line)
    if (line.empty() || line[0] != 'T') return;

    // Check if it's JUST "T" + digits (no other commands on line)
    if (line.length() < 2) return;

    // Extract tool number
    size_t i = 1;
    while (i < line.length() && std::isdigit(line[i])) {
        i++;
    }

    if (i == 1) return;  // No digits after T
    if (i < line.length() && !std::isspace(line[i])) return;  // Not standalone

    std::string tool_str = line.substr(1, i - 1);
    int tool_num = std::stoi(tool_str);

    current_tool_index_ = tool_num;
    spdlog::debug("Tool change: T{}", tool_num);
}
```

Call from main parsing loop (before line 100):

```cpp
// Check for tool changes (T0, T1, T2, etc.)
if (!line.empty() && line[0] == 'T') {
    parse_tool_change_command(line);
    continue;
}
```

#### 1.5 Tag Segments with Tool Index

Modify segment creation (around line 450-460):

```cpp
void GCodeParser::add_segment(const glm::vec3& start, const glm::vec3& end,
                              bool is_extrusion, float e_delta) {
    ToolpathSegment seg;
    seg.start = start;
    seg.end = end;
    seg.is_extrusion = is_extrusion;
    seg.object_name = current_object_;
    seg.extrusion_amount = e_delta;

    // NEW: Tag with current tool
    seg.tool_index = current_tool_index_;

    // NEW: Optional wipe tower tagging
    if (in_wipe_tower_) {
        seg.object_name = "__WIPE_TOWER__";
    }

    segments_.push_back(seg);
}
```

#### 1.6 Expose Color Palette to Consumers

Add public getter:

```cpp
// In gcode_parser.h
class GCodeParser {
public:
    const std::vector<std::string>& get_tool_color_palette() const {
        return tool_color_palette_;
    }
};
```

---

### Phase 2: Geometry Builder Update (REQUIRED)

**File:** `src/gcode_geometry_builder.cpp`

#### 2.1 Store Color Palette in Builder

```cpp
class GCodeGeometryBuilder {
private:
    std::vector<std::string> tool_color_palette_;  // NEW

public:
    void set_tool_color_palette(const std::vector<std::string>& palette) {
        tool_color_palette_ = palette;
    }
};
```

Call after building geometry (wherever `GCodeGeometryBuilder` is instantiated):

```cpp
builder.set_tool_color_palette(parsed_file.get_tool_color_palette());
```

#### 2.2 Add Color Lookup Helper

```cpp
// NEW: Parse hex color to RGB
uint32_t GCodeGeometryBuilder::parse_hex_color(const std::string& hex) const {
    if (hex.length() < 7 || hex[0] != '#') {
        return 0x808080;  // Default gray
    }

    // Parse #RRGGBB format
    unsigned long value = std::strtoul(hex.c_str() + 1, nullptr, 16);

    uint8_t r = (value >> 16) & 0xFF;
    uint8_t g = (value >> 8) & 0xFF;
    uint8_t b = value & 0xFF;

    return (r << 16) | (g << 8) | b;
}

// NEW: Compute color for a segment (replaces old Z-only logic)
uint32_t GCodeGeometryBuilder::compute_segment_color(
    const ToolpathSegment& segment,
    float min_z,
    float max_z) const {

    // Priority 1: Tool-specific color from palette
    if (segment.tool_index >= 0 &&
        segment.tool_index < static_cast<int>(tool_color_palette_.size()) &&
        !tool_color_palette_[segment.tool_index].empty()) {

        return parse_hex_color(tool_color_palette_[segment.tool_index]);
    }

    // Priority 2: Z-height gradient (if enabled)
    if (use_height_gradient_) {
        float mid_z = (segment.start.z + segment.end.z) * 0.5f;
        return compute_color_rgb(mid_z, min_z, max_z);
    }

    // Priority 3: Default filament color
    return (filament_r_ << 16) | (filament_g_ << 8) | filament_b_;
}
```

#### 2.3 Update Ribbon Generation (`gcode_geometry_builder.cpp:390`)

Replace color computation:

```cpp
// OLD:
// uint32_t rgb = compute_color_rgb(mid_z, quant.min_bounds.z, quant.max_bounds.z);

// NEW:
uint32_t rgb = compute_segment_color(segment, quant.min_bounds.z, quant.max_bounds.z);
```

---

### Phase 3: Wipe Tower Exclusion (OPTIONAL - 1 hour)

**File:** `src/gcode_parser.cpp`

#### 3.1 Parse Wipe Tower Markers

Add to metadata comment parsing:

```cpp
void GCodeParser::parse_wipe_tower_marker(const std::string& comment) {
    if (comment.find("WIPE_TOWER_START") != std::string::npos ||
        comment.find("WIPE_TOWER_BRIM_START") != std::string::npos) {
        in_wipe_tower_ = true;
        spdlog::debug("Entering wipe tower section");
    }
    else if (comment.find("WIPE_TOWER_END") != std::string::npos ||
             comment.find("WIPE_TOWER_BRIM_END") != std::string::npos) {
        in_wipe_tower_ = false;
        spdlog::debug("Exiting wipe tower section");
    }
}
```

Call from comment parsing:

```cpp
// In parse_metadata_comment() or main loop
parse_wipe_tower_marker(comment);
```

#### 3.2 Filter in Renderer

Wherever segments are rendered (check `ui_gcode_viewer.cpp` or renderer code):

```cpp
// Skip wipe tower segments if filtering enabled
if (segment.object_name == "__WIPE_TOWER__" && !show_wipe_tower) {
    continue;
}
```

Future enhancement: Add UI checkbox "Show Wipe Tower" (default: off).

---

## Testing Strategy

### Test Cases

1. **Multi-color file (`OrcaCube_ABS_Multicolor.gcode`):**
   - Load file
   - Verify red segments (T0) and beige segments (T2) render correctly
   - Check color transitions happen at T0/T2 boundaries
   - Verify 51 tool changes are reflected in rendering

2. **Single-color file (backward compatibility):**
   - Load existing test file (e.g., `sample.gcode`)
   - Verify still renders correctly (no palette = fallback to default)
   - Confirm Z-gradient mode still works if enabled

3. **Edge cases:**
   - File with no `extruder_colour` metadata → graceful fallback
   - File with empty palette entries (`;;`) → use default color for those tools
   - Non-sequential tools (T0, T2 - skip T1) → correct palette indexing

4. **Wipe tower exclusion (if implemented):**
   - Verify wipe tower segments have `object_name == "__WIPE_TOWER__"`
   - Check filtering works (wipe tower disappears)
   - Verify actual print objects remain visible

### Validation Commands

```bash
# Build and run
make -j
./build/bin/helix-ui-proto -p gcode-test

# Load the multi-color test file in UI
# Visually verify colors match OrcaSlicer preview

# Check logs for tool changes
./build/bin/helix-ui-proto -p gcode-test -vv 2>&1 | grep "Tool change"
# Should see: "Tool change: T0", "Tool change: T2", etc.
```

---

## Code Locations Reference

| Component | File | Lines | What to Change |
|-----------|------|-------|----------------|
| **ToolpathSegment struct** | `include/gcode_parser.h` | 92-99 | Add `tool_index` field |
| **GCodeParser class** | `include/gcode_parser.h` | - | Add tool tracking members |
| **Metadata parsing** | `src/gcode_parser.cpp` | 248-406 | Add `extruder_colour` parsing |
| **Main parsing loop** | `src/gcode_parser.cpp` | 83-124 | Add Tx command detection |
| **Segment creation** | `src/gcode_parser.cpp` | ~450 | Tag segments with tool_index |
| **Color computation** | `src/gcode_geometry_builder.cpp` | 595-658 | Modify `compute_color_rgb()` |
| **Ribbon generation** | `src/gcode_geometry_builder.cpp` | 390-392 | Replace color lookup |

---

## Expected Results

### Before (Current Behavior)
- All segments render in one color (rainbow gradient OR solid color)
- `OrcaCube_ABS_Multicolor.gcode` shows as solid red (first filament)
- Tool changes (`T0`, `T2`) are ignored
- Wipe tower renders like normal geometry

### After (Fixed Behavior)
- Red segments where `T0` is active (`#ED1C24`)
- Beige segments where `T2` is active (`#F4E2C1`)
- Colors change at exact tool change boundaries
- Preview matches OrcaSlicer/PrusaSlicer appearance
- Wipe tower optionally hidden (if Phase 3 implemented)

---

## Troubleshooting

### Colors not changing?
- Check logs: `grep "Tool change" output.log` - should see T0/T1/T2
- Verify palette parsed: `grep "Parsed.*colors" output.log`
- Check segment tool_index values in debugger

### Wrong colors?
- Verify `extruder_colour` metadata format (semicolon-separated)
- Check hex parsing (starts with `#`, 6 hex digits)
- Verify tool index matches palette index

### Crash on load?
- Check palette bounds: `segment.tool_index < palette.size()`
- Handle empty palette gracefully (use default color)

---

## Future Enhancements

1. **UI Controls:**
   - Checkbox: "Show Wipe Tower" (default: off)
   - Checkbox: "Use Tool Colors" vs "Use Z-Gradient"
   - Color picker per tool override

2. **Slicer Compatibility:**
   - Cura: Parse `prime tower` instead of `wipe tower`
   - Simplify3D: May require different metadata format
   - IdeaMaker: Research needed

3. **Performance:**
   - Cache parsed colors (hex → RGB) in palette
   - Pre-compute per-segment colors during geometry build

4. **Advanced Features:**
   - Animate tool changes (highlight active segment)
   - Color legend showing which tool = which color
   - Per-object color override

---

## Summary Checklist

### Phase 1: Parser (REQUIRED)
- [ ] Add `tool_index` field to `ToolpathSegment`
- [ ] Add tool tracking members to `GCodeParser`
- [ ] Parse `extruder_colour` metadata from header
- [ ] Detect `T0`, `T1`, `T2` commands in main loop
- [ ] Tag segments with current tool index
- [ ] Expose color palette via getter

### Phase 2: Geometry Builder (REQUIRED)
- [ ] Pass color palette to builder
- [ ] Add `parse_hex_color()` helper
- [ ] Add `compute_segment_color()` with fallback logic
- [ ] Replace Z-only color computation with palette lookup

### Phase 3: Wipe Tower (OPTIONAL)
- [ ] Parse `WIPE_TOWER_START/END` markers
- [ ] Tag wipe tower segments with special object name
- [ ] Add filtering in renderer

### Testing
- [ ] Load `OrcaCube_ABS_Multicolor.gcode`
- [ ] Verify red and beige colors appear
- [ ] Test single-color file (backward compatibility)
- [ ] Verify Z-gradient mode still works

---

## Estimated Timeline

| Phase | Tasks | Time |
|-------|-------|------|
| **Phase 1** | Parser enhancement (6 tasks) | 3-4 hours |
| **Phase 2** | Geometry builder (4 tasks) | 1-2 hours |
| **Phase 3** | Wipe tower (optional) | 1-2 hours |
| **Testing** | All test cases + validation | 1-2 hours |
| **Total** | All phases | **5-8 hours** |

---

## References

- **Example file:** `/Users/pbrown/code/helixscreen/assets/OrcaCube_ABS_Multicolor.gcode`
- **Parser code:** `src/gcode_parser.cpp`, `include/gcode_parser.h`
- **Builder code:** `src/gcode_geometry_builder.cpp`, `include/gcode_geometry_builder.h`
- **OrcaSlicer G-code format:** Uses semicolon-separated `extruder_colour` metadata
- **Standard G-code:** Tool changes via `T0`, `T1`, `T2` (all firmware)

---

**Document Created:** 2025-11-20
**Status:** Ready for implementation
**Next Step:** Begin Phase 1 - Parser Enhancement
