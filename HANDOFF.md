# Session Handoff Document

**Last Updated:** 2025-10-30 (Session 4)
**Current Focus:** WiFi wizard complete - ready for additional wizard screens

---

## 🎯 Active Work & Next Priorities

### ✅ Recently Completed (Session 2025-10-30 #4)

**WiFi Wizard - Complete** (Session 4 - 2025-10-30)
- ✅ **Fixed padding** - Added responsive `#list_item_padding` constant (4/6/8px)
- ✅ **Fixed styling** - Network items transparent (bg_opa=0, border=0, shadow=0)
- ✅ **Fixed exit crash** - Implemented weak_ptr solution for async callback safety
  - Root cause: `lv_async_call()` queued operations executing after manager freed
  - Solution: `std::weak_ptr<WiFiManager>` with `lock()` checks in callbacks
  - Testing: 20 consecutive exits with NO crashes (verified deterministic fix)
- ✅ **Consolidated font generation** - All icon sizes include lock (U+F023) and WiFi (U+F1EB)
- Files: `src/wifi_manager.cpp`, `src/ui_wizard_wifi.cpp`, `include/wifi_manager.h`, `ui_xml/wifi_network_item.xml`

### ✅ Recently Completed (Previous Sessions)

**Switch Size Parameter Feature** (Completed 2025-10-30)
- ✅ Added semantic `size="tiny|small|medium|large"` parameter to ui_switch
- ✅ Screen-size-aware presets (adapts to TINY/SMALL/LARGE displays)
- ✅ Progressive enhancement: `size="medium" width="100"` (preset + override)
- ✅ Backward compatible: explicit width/height still works
- ✅ Comprehensive unit tests: 600+ lines, 80+ test cases
- ✅ Documentation: QUICK_REFERENCE.md, HANDOFF.md Pattern #3
- ✅ Verified at all screen sizes with screenshots
- Files: `src/ui_switch.cpp`, `tests/unit/test_ui_switch.cpp`, `docs/QUICK_REFERENCE.md`
- Usage: `<ui_switch size="medium"/>` replaces 3-parameter approach

**Responsive Switch Widget** (Completed 2025-10-29)
- ✅ Fixed LVGL XML parser ignoring width/height attributes (manual two-pass parsing)
- ✅ Corrected row heights to account for 20px container padding
- ✅ Production-ready: tested at all three screen sizes
- Files: `src/ui_switch.cpp`, `ui_xml/test_panel.xml`

**WiFi Wizard Screen Loading** (Completed 2025-10-29)
- ✅ Implemented `ui_wizard_load_screen(step)` in `ui_wizard.cpp`
- ✅ WiFi screen now displays instead of black void
- ✅ Fixed title initialization (was "Welcome to Setup", now "WiFi Setup")
- ✅ Responsive constants for WiFi screen (fonts, card heights, toggle height)
- ✅ TINY/SMALL/LARGE all render with appropriate sizing
- Files: `src/ui_wizard.cpp:192-257`, `src/ui_wizard_wifi.cpp:103-166`, `ui_xml/wizard_wifi_setup.xml`

**WiFi Wizard Screen Foundation** (Completed 2025-10-29)
- ✅ Created `ui_wizard_wifi.h/cpp` with subject system (wifi_enabled, wifi_status, ethernet_status)
- ✅ Event callback framework registered (on_wifi_toggle_changed, etc.)
- ✅ Password modal show/hide logic implemented
- ✅ WiFiManager integration hooks ready
- Files: `include/ui_wizard_wifi.h`, `src/ui_wizard_wifi.cpp`

**Wizard Container Redesign** (Completed 2025-10-29)
- ✅ Three-region layout (header/content/footer) with runtime constants
- ✅ Responsive: TINY (480x320), SMALL (800x480), LARGE (1280x720+)
- ✅ Reactive navigation with conditional back button
- ✅ Subject-driven text updates (Next → Finish on last step)
- Files: `ui_xml/wizard_container.xml`, `src/ui_wizard.cpp`

**WiFi Backend System** (Production Ready 2025-10-28)
- macOS (CoreWLAN) + Linux (wpa_supplicant) + Mock backends
- Security hardened, real hardware validated

**WiFi Wizard Reactive Implementation** (Session 2025-10-30 #3 - CORE COMPLETE)
- ✅ **Reactive architecture implemented** - Replaced "pragmatic" manual widget manipulation with subject-driven approach
- ✅ Per-instance subjects created for each network item (ssid, signal_strength, is_secured)
- ✅ LVGL binding API used (`lv_label_bind_text`, `lv_obj_bind_flag_if_eq`) - XML changes require zero C++ updates
- ✅ Proper cleanup - subjects deleted in `NetworkItemData` destructor
- ✅ Fixed duplicate screen initialization (removed redundant `ui_wizard_load_screen` call)
- ✅ Fixed crash on exit (added `ui_wizard_wifi_cleanup()` call before widget deletion)
- ✅ Fixed XML component path format (`A:ui_xml/` not `A:/ui_xml/`)
- ✅ Thread-safety via `lv_async_call()` for backend→UI updates
- Files: `src/ui_wizard_wifi.cpp`, `ui_xml/wifi_network_item.xml`

### Next Priorities

1. **Additional Wizard Screens** (WiFi complete, ready for next screens)
   - Moonraker connection (host/port/API key)
   - Printer identification (32 printer presets in roller)
   - Hardware selection (bed/hotend/fan/LED)
   - Summary/confirmation

2. **Future Enhancements**
   - State persistence between steps
   - Validation and error handling
   - Completion callback integration

---

## 📋 Critical Architecture Patterns (Essential How-To Reference)

### Pattern #0: Flex Layout Height Requirements 🚨 CRITICAL

**When using `flex_grow` on children, parent MUST have explicit height:**

```xml
<!-- BROKEN: Parent has no height -->
<lv_obj flex_flow="row">
    <lv_obj flex_grow="3">Left (30%)</lv_obj>
    <lv_obj flex_grow="7">Right (70%)</lv_obj>
</lv_obj>
<!-- Result: Columns collapse to 0 height -->

<!-- CORRECT: Two-column pattern (30/70 split) -->
<view height="100%" flex_flow="column">
    <lv_obj width="100%" flex_grow="1" flex_flow="column">
        <lv_obj width="100%" flex_grow="1" flex_flow="row">
            <!-- BOTH columns MUST have height="100%" -->
            <lv_obj flex_grow="3" height="100%"
                    flex_flow="column" scrollable="true" scroll_dir="VER">
                <lv_obj height="100">Card 1</lv_obj>
                <lv_obj height="100">Card 2</lv_obj>
            </lv_obj>
            <lv_obj flex_grow="7" height="100%"
                    scrollable="true" scroll_dir="VER">
                <!-- Content -->
            </lv_obj>
        </lv_obj>
    </lv_obj>
</view>
```

**Critical Checks:**
1. Parent has explicit height (`height="300"`, `height="100%"`, or `flex_grow="1"`)
2. ALL columns have `height="100%"` (row height = tallest child)
3. Every level has sizing (wrapper → row → columns)
4. Cards use fixed heights (`height="100"`), NOT `LV_SIZE_CONTENT` in nested flex

**Diagnostic:** Add `style_bg_color="#ff0000"` to visualize bounds

**Reference:** `docs/LVGL9_XML_GUIDE.md:634-716`, `.claude/agents/widget-maker.md:107-149`, `.claude/agents/ui-reviewer.md:101-152`

### Pattern #1: Runtime Constants for Responsive Design

**Use case:** Single XML template that adapts to different screen sizes

```cpp
// C++ - Detect screen size and override constants BEFORE creating XML
int width = lv_display_get_horizontal_resolution(lv_display_get_default());
lv_xml_component_scope_t* scope = lv_xml_component_get_scope("globals");

if (width < 600) {  // TINY
    lv_xml_register_const(scope, "wizard_padding", "6");
    lv_xml_register_const(scope, "wizard_gap", "4");
} else if (width < 900) {  // SMALL
    lv_xml_register_const(scope, "wizard_padding", "12");
    lv_xml_register_const(scope, "wizard_gap", "8");
} else {  // LARGE
    lv_xml_register_const(scope, "wizard_padding", "20");
    lv_xml_register_const(scope, "wizard_gap", "12");
}

// XML - Uses runtime-modified constants
<lv_obj style_pad_all="#wizard_padding" style_pad_column="#wizard_gap">
```

**Why:** One XML template adapts to any screen size without duplication or C++ layout manipulation

**Files:** `ui_wizard.cpp:71-124`, `wizard_container.xml`, `globals.xml:119-125`

### Pattern #2: Wizard Screen Loading 🚨 TODO

**Use case:** Load different wizard screens into wizard_content container based on current step

```cpp
// In ui_wizard.cpp - Add this function:
static void ui_wizard_load_screen(int step) {
    // Find wizard_content container
    lv_obj_t* content = lv_obj_find_by_name(wizard_root, "wizard_content");
    if (!content) {
        spdlog::error("[Wizard] wizard_content container not found");
        return;
    }

    // Clear existing content
    lv_obj_clean(content);

    // Create appropriate screen based on step
    switch (step) {
        case 1:  // WiFi Setup
            ui_wizard_wifi_init_subjects();
            ui_wizard_wifi_create(content);
            ui_wizard_wifi_init_wifi_manager();
            ui_wizard_set_title("WiFi Setup");
            break;
        case 2:  // Moonraker Connection
            // ui_wizard_connection_create(content);
            ui_wizard_set_title("Moonraker Connection");
            break;
        // ... etc for steps 3-7
    }
}

// In ui_wizard_navigate_to_step() - Add this call:
void ui_wizard_navigate_to_step(int step) {
    // ... existing subject updates ...

    // Load screen content (NEW!)
    ui_wizard_load_screen(step);
}
```

**Why:** Wizard container provides empty content area. Screens must be created and parented dynamically.

**Status:** ❌ NOT IMPLEMENTED - wizard shows empty black void currently

**Files:** `ui_wizard.cpp`, `ui_wizard_wifi.cpp`, `wizard_container.xml:75-84`

### Pattern #3: Custom Switch Widget

**Available:** `<ui_switch>` registered for XML use with semantic size presets

```xml
<!-- Recommended: Use semantic size parameter (screen-size-aware) -->
<ui_switch name="my_toggle" size="medium"/>
<ui_switch size="small" checked="true"/>
<ui_switch size="large" orientation="horizontal"/>  <!-- auto|horizontal|vertical -->

<!-- Progressive enhancement: size preset + selective override -->
<ui_switch size="medium" width="100"/>  <!-- Custom width, keeps medium height/knob_pad -->

<!-- Backward compatible: explicit sizing still works -->
<ui_switch width="64" height="32" knob_pad="2"/>
```

**Size Parameter:** `tiny`, `small`, `medium`, `large` (adapts to TINY/SMALL/LARGE screens)

**Supports:** All standard `lv_obj` properties (width, height, style_*, etc.)

**Why use size presets:** 1 parameter instead of 3 (width + height + knob_pad), responsive by default

**Files:** `include/ui_switch.h`, `src/ui_switch.cpp`, `docs/QUICK_REFERENCE.md`

### Pattern #4: Navigation History Stack

**When to use:** Overlay panels (motion, temps, extrusion, keypad)

```cpp
ui_nav_push_overlay(motion_panel);  // Shows overlay, saves history
if (!ui_nav_go_back()) { /* fallback */ }
```

**Files:** `ui_nav.h:54-62`, `ui_nav.cpp:250-327`

### Pattern #5: Global Keyboard for Textareas

```cpp
// One-time init in main.cpp (already done)
ui_keyboard_init(lv_screen_active());

// For each textarea
ui_keyboard_register_textarea(my_textarea);  // Auto show/hide on focus
```

**Files:** `include/ui_keyboard.h`, `src/ui_keyboard.cpp`

### Pattern #6: Subject Initialization Order

**MUST initialize subjects BEFORE creating XML:**

```cpp
lv_xml_register_component_from_file("A:/ui_xml/my_panel.xml");
ui_my_panel_init_subjects();  // FIRST
lv_xml_create(screen, "my_panel", NULL);  // AFTER
```

### Pattern #7: Component Instantiation Names

**Always add explicit `name` attributes:**

```xml
<!-- WRONG --><my_panel/>
<!-- CORRECT --><my_panel name="my_panel"/>
```

**Why:** Component `<view name="...">` doesn't propagate to instantiation

### Pattern #8: Image Scaling in Flex Layouts

```cpp
lv_obj_update_layout(container);  // Force layout calculation FIRST
ui_image_scale_to_cover(img, container);
```

**Why:** LVGL uses deferred layout - containers report 0x0 until forced

**Files:** `ui_utils.cpp:213-276`, `ui_panel_print_status.cpp:249-314`

### Pattern #9: Logging Policy

**ALWAYS use spdlog, NEVER printf/cout/LV_LOG:**

```cpp
#include <spdlog/spdlog.h>
spdlog::info("Operation complete: {}", value);  // fmt-style formatting
spdlog::error("Failed: {}", (int)enum_val);     // Cast enums
```

**Reference:** `CLAUDE.md:77-134`

### Pattern #10: Copyright Headers

**ALL new files MUST include GPL v3 header**

**Reference:** `docs/COPYRIGHT_HEADERS.md`

### Pattern #11: UI Testing Infrastructure

**See `docs/UI_TESTING.md` for complete UI testing guide**

**Quick reference:**
- `UITest::click(widget)` - Simulate touch at widget center
- `UITest::find_by_name(parent, "name")` - Widget lookup
- `UITest::wait_until(condition, timeout)` - Async condition wait
- Run tests: `./build/bin/run_tests "[tag]"`

**Files:** `tests/ui_test_utils.h/cpp`, `tests/unit/test_wizard_wifi_ui.cpp`

### Pattern #12: Thread-Safety with lv_async_call() ⚠️ CRITICAL

**LVGL is NOT thread-safe.** Backend threads (WiFi, networking, file I/O) cannot create/modify widgets directly.

**Solution:** Use `lv_async_call()` to dispatch UI updates to main thread

**See ARCHITECTURE.md "Thread Safety" section for:**
- Complete code example with CallbackData struct pattern
- When to use vs. when subjects are sufficient
- Memory management best practices
- Reference implementation in `src/wifi_manager.cpp:102-190`

**Quick rule:** If backend callback needs to call ANY `lv_obj_*()` function, wrap it in `lv_async_call()`

---

## 🔧 Known Issues & Gotchas

### Node.js/npm Dependency for Icon Generation 📦

**Current State:** canvas 3.2.0 requires Node.js v22 compatibility

**Dependencies:**
- Node.js v22.20.0+ (for canvas pre-built binaries)
- npm packages: lv_font_conv, lv_img_conv
- Canvas native dependencies: cairo, pango, libpng, libjpeg, librsvg

**When Required:** Only when regenerating fonts/icons (`make generate-fonts`, `make material-icons-convert`)

**Phase 2 TODO:** Make npm optional for regular builds (check if fonts exist before requiring lv_font_conv)

**Reference:** `package.json`, `mk/fonts.mk`, Phase 1 commit `83a867a`

### UI Testing Known Issues 🐛

**See `docs/UI_TESTING.md` for comprehensive testing documentation**

**Critical Issues:**
1. Multiple fixture instances cause segfaults (only 1/10 WiFi tests passing)
2. Virtual input events don't trigger ui_switch VALUE_CHANGED events

**Status:** UI testing is a deferred project - documented for future work

### LVGL 9 XML Roller Options ⚠️ WORKAROUND

**Problem:** LVGL 9 XML roller parser fails with `options="'item1\nitem2' normal"` syntax

**Workaround:** Set roller options programmatically in C++:
```cpp
lv_roller_set_options(roller, "Item 1\nItem 2\nItem 3", LV_ROLLER_MODE_NORMAL);
```

**Status:** Applied to wizard step 3 printer selection (32 printer types)

**Files:** `src/ui_wizard.cpp:352-387`

### LVGL 9 XML Flag Syntax ✅ FIXED

**NEVER use `flag_` prefix:**
- ❌ `flag_hidden="true"` → ✅ `hidden="true"`
- ❌ `flag_clickable="true"` → ✅ `clickable="true"`

**Status:** All XML files fixed (2025-10-24)

### LV_SIZE_CONTENT in Nested Flex

**Problem:** Evaluates to 0 before `lv_obj_update_layout()` is called

**Solutions:**
1. Call `lv_obj_update_layout()` after creation (timing sensitive)
2. Use explicit pixel dimensions (recommended)
3. Use `style_min_height`/`style_min_width` for cards

**Reference:** `docs/LVGL9_XML_GUIDE.md:705-708`

---

**Rule:** When work is complete, REMOVE it from HANDOFF immediately. Keep this document lean and current.
