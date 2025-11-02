# Session Handoff Document

**Last Updated:** 2025-11-02
**Current Focus:** Wizard baseline implementations COMPLETE - All 8 steps functional

---

## ✅ JUST COMPLETED: Real File Operations Integration (2025-11-02)

**Print Select Panel now fetches real data from Moonraker**

**What Works:**
- `ui_panel_print_select_refresh_files()` fetches real file list via `MoonrakerAPI::list_files()`
- Nested async metadata fetching for each file via `get_file_metadata()`
- Extracts print time (seconds → minutes) and filament weight (grams)
- Cards/list rows update automatically as metadata arrives
- Thumbnail URL construction (`construct_thumbnail_url()`) - logs URLs, download deferred

**Files Modified:**
- `src/ui_panel_print_select.cpp` - Added config.h include, construct_thumbnail_url() helper, enhanced refresh_files() with nested callbacks

**Edge Cases Handled:**
- Bounds checking (file_list changes during async ops)
- Missing metadata → keeps placeholders (0 min, 0g)
- Error callbacks log warnings without crashing
- Empty file lists → shows existing empty state UI

**What's Deferred:**
- Actual HTTP thumbnail downloads (URL construction works, marked TODO)
- Would need: libhv HttpClient integration, temp directory, file cleanup

**Testing:**
- ✅ Builds cleanly (zero warnings)
- ✅ Mock mode (`--test`) still works with test data
- ⏳ Real Moonraker test requires live printer (code ready)

**Key Pattern - Async Metadata Loading:**
```cpp
api->list_files("gcodes", "", false, [api](const std::vector<FileInfo>& files) {
    // Show files immediately with placeholders
    for (const auto& file : files) { /* add to file_list */ }
    populate_card_view();

    // Fetch metadata asynchronously
    for (size_t i = 0; i < file_list.size(); i++) {
        api->get_file_metadata(filename, [i, filename](const FileMetadata& m) {
            // Bounds check, update file_list[i], re-render
        });
    }
});
```

---

## ✅ JUST COMPLETED: Wizard Baseline Implementations (2025-11-02)

**All 8 wizard steps now have functional baseline C++ implementations!**

### Wizard Framework (Complete)
- Subject-based navigation (current_step, total_steps, wizard_title, wizard_progress)
- Responsive constants system (SMALL ≤480, MEDIUM 481-800, LARGE >800px)
- Back/Next navigation with automatic "Finish" button on step 8
- Parent-defined constants pattern (wizard_container → child screens)
- Theme-aware background colors
- Next button control (enabled/disabled per screen)
- total_steps = 8 (fixed throughout codebase)

### All Steps Implemented ✅

**Step 1: WiFi Setup** (ui_wizard_wifi.h/.cpp)
- WiFi toggle, network scanning, password modal
- Backend integration with WiFiManager (mock + real)

**Step 2: Moonraker Connection** (ui_wizard_connection.h/.cpp)
- IP/hostname + port validation
- WebSocket connection testing with status feedback
- Config persistence to `/moonraker/host` and `/moonraker/port`
- Next button disabled until successful connection
- Comprehensive unit tests (validation + UI)

**Step 3: Printer Identification** (ui_wizard_printer_identify.h/.cpp)
- Printer name textarea
- Printer type roller (33 options: Voron, Prusa, Creality, Ender, Bambu, etc.)
- Config persistence to `/printer/name` and `/printer/type`
- Auto-detection status display (ready for Moonraker integration)

**Step 4: Bed Select** (ui_wizard_bed_select.h/.cpp)
- Bed heater dropdown (placeholder options)
- Bed sensor dropdown (placeholder options)
- Config persistence to `/printer/bed/heater` and `/printer/bed/sensor`

**Step 5: Hotend Select** (ui_wizard_hotend_select.h/.cpp)
- Extruder heater dropdown
- Temperature sensor dropdown
- Config persistence to `/printer/extruder/heater` and `/printer/extruder/sensor`

**Step 6: Fan Select** (ui_wizard_fan_select.h/.cpp)
- Hotend fan dropdown
- Part cooling fan dropdown
- Config persistence to `/printer/fans/hotend_fan` and `/printer/fans/part_cooling_fan`

**Step 7: LED Select** (ui_wizard_led_select.h/.cpp)
- LED strip dropdown (optional)
- Config persistence to `/printer/led/strip`

**Step 8: Summary** (ui_wizard_summary.h/.cpp)
- Read-only display of all configurations
- Loads from config: printer name, type, network address, hardware selections
- "Finish" button replaces "Next" automatically

### Files Created (12 new files)
- `include/ui_wizard_printer_identify.h`, `src/ui_wizard_printer_identify.cpp`
- `include/ui_wizard_bed_select.h`, `src/ui_wizard_bed_select.cpp`
- `include/ui_wizard_hotend_select.h`, `src/ui_wizard_hotend_select.cpp`
- `include/ui_wizard_fan_select.h`, `src/ui_wizard_fan_select.cpp`
- `include/ui_wizard_led_select.h`, `src/ui_wizard_led_select.cpp`
- `include/ui_wizard_summary.h`, `src/ui_wizard_summary.cpp`

### Files Modified
- `src/ui_wizard.cpp` - Wired all 8 steps, updated total_steps to 8
- `src/main.cpp` - Updated CLI arg validation to allow steps 1-8
- `include/ui_wizard.h` - Added ui_wizard_set_next_button_enabled()
- `ui_xml/wizard_container.xml` - Added Next button control via subject

### Next Priorities

1. **Integrate real Moonraker data for Steps 4-6**
   - Replace placeholder dropdown options with live Moonraker heater/sensor/fan queries
   - Use `MoonrakerAPI::query_printer_objects()` to fetch available components
   - Dynamically populate dropdowns based on printer.cfg

2. **Implement Finish button handler**
   - Save wizard completion flag to config
   - Trigger transition to main application screen
   - Handle "complete wizard" vs "skip wizard" paths

3. **Add printer auto-detection to Step 3**
   - Query Moonraker for printer info after connection (Step 2)
   - Auto-populate printer name/type if detectable
   - Allow manual override

**Key Files:**
- `src/ui_wizard.cpp` - Main navigation, loads all 8 steps
- `src/ui_wizard_connection.cpp` - Step 2 with comprehensive tests
- `src/ui_wizard_printer_identify.cpp` - Step 3 with 33 printer types
- `src/ui_wizard_{bed,hotend,fan,led}_select.cpp` - Steps 4-7 hardware config
- `src/ui_wizard_summary.cpp` - Step 8 final review
- `ui_xml/wizard_*.xml` - All 8 XML layouts (complete)
- `include/wizard_validation.h` - Input validation helpers
- `tests/unit/test_wizard_connection*.cpp` - Comprehensive test suite

---

## 📋 Critical Patterns for Wizard Work

### Pattern #1: Flex Layout Height Requirements ⚠️

**flex_grow children require parent with explicit height:**
```xml
<!-- ❌ BROKEN: Parent has no height -->
<lv_obj flex_flow="row">
    <lv_obj flex_grow="3">Left</lv_obj>
    <lv_obj flex_grow="7">Right</lv_obj>
</lv_obj>

<!-- ✅ CORRECT: Parent has height, columns have height="100%" -->
<lv_obj height="100%" flex_flow="row">
    <lv_obj flex_grow="3" height="100%">Left</lv_obj>
    <lv_obj flex_grow="7" height="100%">Right</lv_obj>
</lv_obj>
```
**Fix:** Add `style_bg_color="#ff0000"` to visualize bounds. See `docs/LVGL9_XML_GUIDE.md:634-716`

### Pattern #2: Runtime Constants (Responsive + Theme) ⚠️

**Wizard uses heavily for screen size adaptation:**
```cpp
// Detect size, override constants BEFORE creating XML
int width = lv_display_get_horizontal_resolution(lv_display_get_default());
lv_xml_component_scope_t* scope = lv_xml_component_get_scope("wizard_container");

if (width < 600) {
    lv_xml_register_const(scope, "wizard_padding", "6");
} else {
    lv_xml_register_const(scope, "wizard_padding", "20");
}
```
**See:** `ui_wizard.cpp:90-203` for complete wizard responsive pattern

### Pattern #3: Subject Initialization Order ⚠️

**Always init subjects BEFORE creating XML:**
```cpp
lv_xml_register_component_from_file("A:/ui_xml/my_panel.xml");
ui_my_panel_init_subjects();  // FIRST
lv_xml_create(screen, "my_panel", NULL);  // AFTER
```

### Pattern #4: Test Mode ⚠️

**Wizard will use test mode - never auto-fallback to mocks:**
```cpp
if (config.should_mock_wifi()) {
    return std::make_unique<WifiBackendMock>();
}
// Production: return nullptr if hardware fails, NO fallback
```
**See:** `test_config.h` for complete API

---

## 🔧 Known Issues & Gotchas

### LVGL 9 XML Roller Options

**Issue:** XML parser fails with `options="'item1\nitem2' normal"` syntax

**Workaround:** Set roller options programmatically:
```cpp
lv_roller_set_options(roller, "Item 1\nItem 2\nItem 3", LV_ROLLER_MODE_NORMAL);
```

**Files:** `src/ui_wizard.cpp:352-387`

---

**Rule:** When work is complete, DELETE it from HANDOFF immediately. Keep this document lean and current.
