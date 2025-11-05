# Session Handoff Document

**Last Updated:** 2025-11-04
**Current Focus:** WiFi password modal complete, ready for wizard polish

---

## ✅ CURRENT STATE

### Completed Phases

1. **WiFi Password Modal** - ✅ COMPLETE
   - Modal appears and functions correctly
   - Connect button disables immediately on click (50% opacity)
   - Fixed button radius morphing on press (disabled LV_THEME_DEFAULT_GROW in lv_conf.h)
   - Connected network highlighted with primary color border + distinct background
   - Status text reads from XML constants ("WiFi Enabled", "Connecting to...", etc.)
   - Password validation working (empty password → error, wrong password → AUTH_FAILED)
   - Fixed critical blocking bug in wifi_backend_mock.cpp (thread.join() → thread.detach())

2. **Global Disabled State Styling** - ✅ COMPLETE
   - All widgets automatically dim to 50% opacity when disabled
   - Implemented in theme system (helix_theme.c)
   - No per-widget styling needed - applies to buttons, inputs, all interactive elements

3. **Custom HelixScreen Theme** - ✅ COMPLETE
   - Implemented custom LVGL wrapper theme (helix_theme.c) that extends default theme
   - Input widgets (textarea, dropdown, roller, spinbox) get computed background colors automatically
   - Dark mode: input backgrounds 22-27 RGB units lighter than cards (#35363A vs #1F1F1F)
   - Light mode: input backgrounds 22-27 RGB units darker than cards (#DADCDE vs #F0F3F9)
   - Removed 273 lines of fragile LVGL private API patching from ui_theme.cpp
   - Uses LVGL's public theme API, much more maintainable across LVGL versions

4. **Phase 1: Hardware Discovery Trigger** - ✅ COMPLETE
   - Wizard triggers `discover_printer()` after successful connection
   - Connection stays alive for hardware selection steps 4-7

5. **Phase 2: Dynamic Dropdown Population** - ✅ COMPLETE
   - All 4 wizard hardware screens use dynamic dropdowns from MoonrakerClient
   - Hardware filtering: bed (by "bed"), hotend (by "extruder"/"hotend"), fans (separated by type), LEDs (all)
   - Fixed critical layout bug: `height="LV_SIZE_CONTENT"` → `flex_grow="1"`

6. **Phase 2.5: Connection Screen Reactive UI** - ✅ SUBSTANTIALLY COMPLETE
   - Reactive Next button control via connection_test_passed subject
   - Split connection_status into icon (checkmark/xmark) and text subjects
   - Virtual keyboard integration with auto-show on textarea focus
   - Config prefilling for IP/port from previous sessions
   - Disabled button styling (50% opacity when connection_test_passed=0)

7. **Phase 3: Mock Backend** - ✅ COMPLETE
   - `MoonrakerClientMock` with 7 printer profiles, factory pattern in main.cpp

### What Works Now

- ✅ Wizard connection screen (step 2) with reactive Next button control
- ✅ Virtual keyboard with screen slide animation and auto-show on focus
- ✅ Wizard hardware selection (steps 4-7) dynamically populated from discovered hardware
- ✅ Mock backend testing (`--test` flag)
- ✅ Config persistence for all wizard fields
- ✅ WiFi wizard screen with password modal, connection, and status display

### What Needs Work

- ⚠️ Wizard steps 1-2 need polish and refinement (basic functionality complete)
- ❌ Wizard steps 3 and 8 (printer ID, summary) need implementation
- ❌ Real printer testing with connection screen (only tested with mock backend)
- ❌ Printer auto-detection via mDNS (future enhancement)

---

## 🚀 NEXT PRIORITIES

### 1. Polish Wizard Connection Screen (Step 2)
- Improve error messaging and user feedback
- Add timeout handling for connection attempts
- Test with real printer (no `--test` flag)
- Refine layout and visual feedback

### 2. Implement Printer Identification (Step 3)
- Allow user to name their printer
- Store printer name in config
- Validate input (non-empty, reasonable length)

---

## 📋 Critical Patterns Reference

### Pattern #0: LV_SIZE_CONTENT Bug

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

**Why:** LV_SIZE_CONTENT doesn't work reliably when child elements are themselves flex containers or have complex layouts. Use `flex_grow` or fixed heights instead.

### Pattern #1: Thread Management - NEVER Block UI Thread

**CRITICAL:** NEVER use blocking operations like `thread.join()` in code paths triggered by UI events.

```cpp
// ❌ WRONG - Blocks LVGL main thread for 2-3 seconds
if (connect_thread_.joinable()) {
    connect_thread_.join();  // UI FREEZES HERE
}

// ✅ CORRECT - Non-blocking cleanup
connect_active_ = false;  // Signal thread to exit
if (connect_thread_.joinable()) {
    connect_thread_.detach();  // Let thread finish on its own
}
```

**Why:** Blocking the LVGL main thread prevents all UI updates (including immediate visual feedback like button states). Use detach() or async patterns instead.

**Symptoms of blocking:**
- UI changes delayed by seconds
- Direct style manipulation (`lv_obj_set_style_*`) also delayed
- Button states don't update until blocking call completes

### Pattern #2: Global Disabled State Styling

All widgets automatically get 50% opacity when disabled via theme system. No per-widget styling needed.

```cpp
// Enable/disable any widget
lv_obj_add_state(widget, LV_STATE_DISABLED);    // Dims to 50% automatically
lv_obj_remove_state(widget, LV_STATE_DISABLED); // Restores full opacity
```

**Implementation:** helix_theme.c applies disabled_style globally to all objects for LV_STATE_DISABLED.

### Pattern #3: LVGL XML String Constants

Use `<str>` tags for C++-accessible string constants, NOT `<enumdef>`.

```xml
<!-- ✅ CORRECT - String constants -->
<consts>
  <str name="wifi_status.disabled" value="WiFi Disabled"/>
  <str name="wifi_status.enabled" value="WiFi Enabled"/>
  <str name="wifi_status.connecting" value="Connecting to "/>
</consts>
```

```cpp
// Access via component scope
lv_xml_component_scope_t* scope = lv_xml_component_get_scope("wizard_wifi_setup");
const char* text = lv_xml_get_const(scope, "wifi_status.enabled");
// Returns: "WiFi Enabled"
```

**Why:** `<enumdef>` is ONLY for widget property types in `<api>` sections, not for string lookups.

### Pattern #4: Dynamic Dropdown Population

```cpp
// Store items for event callback mapping
static std::vector<std::string> hardware_items;

// Build options (newline-separated), filter hardware, add "None"
hardware_items.clear();
std::string options_str;
for (const auto& item : client->get_heaters()) {
    if (item.find("bed") != std::string::npos) {
        hardware_items.push_back(item);
        if (!options_str.empty()) options_str += "\n";
        options_str += item;
    }
}
hardware_items.push_back("None");
if (!options_str.empty()) options_str += "\n";
options_str += "None";

lv_dropdown_set_options(dropdown, options_str.c_str());

// Event callback
static void on_changed(lv_event_t* e) {
    int idx = lv_dropdown_get_selected(dropdown);
    if (idx < hardware_items.size()) config->set("/printer/component", hardware_items[idx]);
}
```

### Pattern #5: Moonraker Client Access

```cpp
#include "app_globals.h"
#include "moonraker_client.h"

MoonrakerClient* client = get_moonraker_client();
if (!client) return;  // Graceful degradation

const auto& heaters = client->get_heaters();
const auto& sensors = client->get_sensors();
const auto& fans = client->get_fans();
const auto& leds = client->get_leds();
```

### Pattern #6: Reactive Button Control via Subjects

Control button state (enabled/disabled, styled) reactively using subjects and bind_flag_if_eq.

```cpp
// C++ - Initialize subject to control button state
lv_subject_t connection_test_passed;
lv_subject_init_int(&connection_test_passed, 0);  // 0 = disabled
```

```xml
<!-- XML - Bind button clickable flag and style to subject -->
<lv_button name="wizard_next_button">
  <!-- Disable clickable when connection_test_passed == 0 -->
  <lv_obj-bind_flag_if_eq subject="connection_test_passed" flag="clickable" ref_value="0" negate="true"/>
  <!-- Apply disabled style when connection_test_passed == 0 -->
  <lv_obj-bind_flag_if_eq subject="connection_test_passed" flag="user_1" ref_value="0"/>
</lv_button>

<!-- Define disabled style -->
<lv_style selector="LV_STATE_USER_1" style_opa="128"/>  <!-- 50% opacity -->
```

```cpp
// C++ - Update subject to enable button
lv_subject_set_int(&connection_test_passed, 1);  // Button becomes enabled
```

**Why:** Fully reactive UI - no manual button state management. Button automatically updates when subject changes.

### Pattern #7: Button Radius Morphing Fix

Fixed 8px radius on buttons appears to morph when `LV_THEME_DEFAULT_GROW=1` causes 3px width/height transform on press. Disable in lv_conf.h:690 to prevent visual artifact while preserving other animations.

---

**Reference:** See `docs/MOONRAKER_HARDWARE_DISCOVERY_PLAN.md` for implementation plan

**Next Session:** Polish wizard connection screen (step 2), then implement printer identification (step 3)
