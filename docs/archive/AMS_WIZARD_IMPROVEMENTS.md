# AMS Wizard & Filament Sensor Improvements

## Issue 1: AMS Wizard - Polish the UI

**Problems:**
1. Shows "Text" placeholder instead of integration message (wrong XML attribute)
2. Missing AMS type icon/logo
3. Missing buffer name (e.g., "4 lanes • Turtle_1" not just "4 lanes detected")
4. Poor justification/layout

**Reference:** `src/ui/ui_panel_ams.cpp:50` has `get_ams_logo_path()` that maps AMS types to logos

### Files to Modify

| File | Changes |
|------|---------|
| `ui_xml/wizard_ams_identify.xml` | Add logo, reactive bindings, fix text attribute |
| `include/ui_wizard_ams_identify.h` | Add subjects + static buffers |
| `src/ui/ui_wizard_ams_identify.cpp` | Init subjects, register, update via subjects, use `AmsState::get_logo_path()` |
| `include/ams_state.h` | Add `static const char* get_logo_path(const std::string&)` |
| `src/ams/ams_state.cpp` | Implement `get_logo_path()` (move from ui_panel_ams.cpp) |
| `src/ui/ui_panel_ams.cpp` | Use `AmsState::get_logo_path()` instead of local function |

### XML Layout (New Design - with reactive bindings)

```xml
<ui_card width="100%" flex_grow="1" style_radius="#border_radius_large"
         style_pad_all="#space_lg" style_pad_gap="#space_md"
         flex_flow="column" style_flex_main_place="center" style_flex_cross_place="center">

  <!-- Logo image - set programmatically based on AMS type -->
  <lv_image name="ams_logo" width="#space_2xl" height="#space_2xl" inner_align="contain"/>

  <!-- AMS Type name - REACTIVE BINDING -->
  <text_heading name="ams_type_label" bind_text="wizard_ams_type" style_text_align="center"/>

  <!-- Details: "4 lanes • Turtle_1" - REACTIVE BINDING -->
  <text_body name="ams_details_label" bind_text="wizard_ams_details"
             style_text_color="#text_secondary" style_text_align="center"/>

  <lv_obj height="#space_md" width="1" style_bg_opa="0" style_border_width="0"/>

  <!-- Integration message - static text (use text= attribute!) -->
  <text_body text="HelixScreen will integrate with your filament changer for multi-material print operations."
             style_text_align="center" style_text_color="#text_secondary" width="90%"/>
</ui_card>
```

### C++ Changes (`ui_wizard_ams_identify.cpp`)

**Add subjects for reactive bindings:**

```cpp
// In header:
lv_subject_t wizard_ams_type_;
lv_subject_t wizard_ams_details_;
static char ams_type_buffer_[64];
static char ams_details_buffer_[128];

// In init_subjects():
lv_subject_init_string(&wizard_ams_type_, ams_type_buffer_, "Unknown");
lv_subject_init_string(&wizard_ams_details_, ams_details_buffer_, "");

// Register subjects globally for XML binding:
lv_subject_registry_register("wizard_ams_type", &wizard_ams_type_);
lv_subject_registry_register("wizard_ams_details", &wizard_ams_details_);
```

**Update display via subjects (not lv_label_set_text):**

```cpp
void WizardAmsIdentifyStep::update_display() {
    auto& ams = AmsState::instance();
    AmsBackend* backend = ams.get_backend();

    // Set type name via subject
    std::string type_name = get_ams_type_name();
    lv_subject_copy_string(&wizard_ams_type_, type_name.c_str());

    // Set details via subject
    std::string details = get_ams_details();
    lv_subject_copy_string(&wizard_ams_details_, details.c_str());

    // Set logo (still imperative - images don't have bind support)
    lv_obj_t* logo = lv_obj_find_by_name(screen_root_, "ams_logo");
    if (logo && backend) {
        const char* logo_path = AmsState::get_logo_path(backend->get_system_info().type_name);
        if (logo_path && logo_path[0] != '\0') {
            lv_image_set_src(logo, logo_path);
            lv_obj_remove_flag(logo, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(logo, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
```

**get_ams_details() with buffer name:**
```cpp
std::string WizardAmsIdentifyStep::get_ams_details() const {
    auto& ams = AmsState::instance();
    AmsBackend* backend = ams.get_backend();
    if (!backend) return "System detected";

    AmsSystemInfo info = backend->get_system_info();
    std::string details;

    if (info.total_slots > 0) {
        details = std::to_string(info.total_slots) + " lanes";
    }

    // Add unit name if available (e.g., "• Turtle_1")
    if (!info.units.empty() && !info.units[0].name.empty()) {
        if (!details.empty()) details += " • ";
        details += info.units[0].name;
    }

    return details.empty() ? "System detected" : details;
}
```

**Move `get_ams_logo_path()` to `AmsState`:**
- Add as static method: `static const char* AmsState::get_logo_path(const std::string& type_name)`
- Remove from `src/ui/ui_panel_ams.cpp`
- Both `ui_panel_ams.cpp` and `ui_wizard_ams_identify.cpp` use `AmsState::get_logo_path()`

---

## Issue 2: Filament Sensor - Add Guessing Heuristics

**Problem:** Runout sensor dropdown defaults to "None" even when good candidates exist (e.g., `tool_start`).

**Solution:** Add `guess_runout_sensor()` to `PrinterHardware` class, following the existing pattern for fans/heaters.

### Files to Modify

| File | Changes |
|------|---------|
| `include/printer_hardware.h` | Add `guess_runout_sensor()` declaration |
| `src/printer/printer_hardware.cpp` | Implement guessing logic |
| `src/ui/ui_wizard_filament_sensor_select.cpp` | Call guess function to pre-select |

### Guessing Priority (highest to lowest)

```cpp
std::string PrinterHardware::guess_runout_sensor(
    const std::vector<std::string>& filament_sensors) const {

    // Priority 1: Exact "runout_sensor" or "filament_runout"
    if (has_exact(filament_sensors, "runout_sensor")) return "runout_sensor";
    if (has_exact(filament_sensors, "filament_runout")) return "filament_runout";

    // Priority 2: Contains "runout"
    auto match = find_containing(filament_sensors, "runout");
    if (!match.empty()) return match;

    // Priority 3: Contains "tool_start" (AFC pattern - filament at toolhead)
    match = find_containing(filament_sensors, "tool_start");
    if (!match.empty()) return match;

    // Priority 4: Contains "filament" (generic)
    match = find_containing(filament_sensors, "filament");
    if (!match.empty()) return match;

    // Priority 5: Contains "switch" or "motion" (sensor types)
    match = find_containing(filament_sensors, "switch");
    if (!match.empty()) return match;
    match = find_containing(filament_sensors, "motion");
    if (!match.empty()) return match;

    return "";  // No guess
}
```

### Integration in Wizard

In `ui_wizard_filament_sensor_select.cpp::create()`, after `filter_standalone_sensors()`:

```cpp
// Try to guess best runout sensor if none configured
bool has_configured_runout = false;
for (const auto& sensor : standalone_sensors_) {
    if (sensor.role == helix::FilamentSensorRole::RUNOUT) {
        has_configured_runout = true;
        break;
    }
}

if (!has_configured_runout && !standalone_sensors_.empty()) {
    // Build list of sensor names for guessing
    std::vector<std::string> sensor_names;
    for (const auto& s : standalone_sensors_) {
        sensor_names.push_back(s.sensor_name);
    }

    // Get printer hardware for guessing
    MoonrakerClient* client = get_moonraker_client();
    if (client) {
        auto hw = client->make_printer_hardware();
        std::string guess = hw->guess_runout_sensor(sensor_names);

        // Find index in sensor_items_ and set subject
        for (size_t i = 0; i < sensor_items_.size(); i++) {
            if (sensor_items_[i] == guess) {
                lv_subject_set_int(&runout_sensor_selected_, static_cast<int>(i));
                spdlog::info("[{}] Auto-selected runout sensor: {}", get_name(), guess);
                break;
            }
        }
    }
}
```

---

## Verification

```bash
make -j && ./scripts/screenshot.sh helix-screen wizard-sensors wizard_filament_sensor_select
```

Test against Voron @ 192.168.1.112 which has `tool_start` and `tool_end` sensors.
