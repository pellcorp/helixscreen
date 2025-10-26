# Session Handoff Document

**Last Updated:** 2025-10-26 Late Evening
**Current Focus:** Real Printer Connection Working - Ready for First-Run Wizard Implementation

---

## Recent Work (2025-10-26 Late Evening)

### Real Printer Connection Testing + First-Run Wizard Planning ✅ COMPLETE

**Successfully connected to real Voron V2 printer at 192.168.1.112!**

**Bug Fixes:**

1. **Async Timing Issue** (`src/main.cpp:633-668`):
   - **Problem**: Called `discover_printer()` before WebSocket actually connected
   - **Fix**: Moved discovery into `on_connected` callback

2. **Lambda Capture Error** (`src/main.cpp:637`):
   - **Problem**: Tried to capture static `printer_state` by reference
   - **Fix**: Removed capture (static vars don't need capture)

3. **JSON-RPC Params Bug** (`src/moonraker_client.cpp:157-171`):
   - **Problem**: Moonraker rejected empty params object `"params": {}`
   - **Fix**: Only include params field if not null/empty:
     ```cpp
     if (!params.is_null() && !params.empty()) {
         rpc["params"] = params;
     }
     ```

**Real Printer Test Results:**
- Connected to ws://192.168.1.112:7125/websocket
- Moonraker v0.9.3, hostname: voronv2
- Discovered: 2 heaters, 6 sensors, 6 fans, 3 LEDs
- 22 objects subscribed successfully

**First-Run Wizard Planning:**
- Added Phase 11 to ROADMAP.md with comprehensive requirements
- Wizard screens: Connection → Hardware Mapping → Summary → Save
- Auto-default behavior when only one component exists
- mDNS optional (not universally enabled), manual IP primary method

**Files Modified:**
- `helixconfig.json` - Changed moonraker_host to 192.168.1.112
- `src/main.cpp` - Fixed async callback timing
- `src/moonraker_client.cpp` - Fixed JSON-RPC params handling
- `docs/ROADMAP.md` - Added Phase 11 (First-Run Configuration Wizard)

### Earlier: Moonraker Integration Foundation ✅ COMPLETE (2025-10-26 Morning)
- Integrated libhv WebSocket library (static linking via parent repo)
- Created `MoonrakerClient` wrapper class with JSON-RPC support
- Created `PrinterState` reactive state manager with LVGL subjects
- Cross-platform build system (macOS/Linux-aware NPROC + linker flags)
- Increased `LV_DRAW_THREAD_STACK_SIZE` to 32KB (eliminates warning)
- Files: `include/moonraker_client.h`, `src/moonraker_client.cpp`, `include/printer_state.h`, `src/printer_state.cpp`, `Makefile`, `lv_conf.h`

---

## Project Status

**All UI components complete. Infrastructure ready for Moonraker integration.**

Navigation system robust. All panels render correctly across all screen sizes. Reactive state management infrastructure in place with LVGL subjects.

### What Works
- ✅ Navigation system with history stack
- ✅ All UI panels functional with mock data
- ✅ Responsive design (480×320 to 1280×720)
- ✅ Material Design icons with dynamic recoloring
- ✅ **Config** - JSON-based configuration with auto-migration
- ✅ **MoonrakerClient** - WebSocket client with auto-discovery
- ✅ **PrinterState** - Reactive state manager with subjects
- ✅ **Cross-platform build** - macOS/Linux-aware Makefile
- ✅ **Connection on Startup** - Connects and discovers printer automatically

### Next Steps (Priority Order)
1. 🎯 **Implement First-Run Wizard** (Phase 11 in ROADMAP.md)
   - Connection screen with IP/port entry
   - Hardware mapping screens (bed, hotend, fans, LEDs)
   - Config validation and storage
   - Settings panel integration for re-running wizard

2. 🔌 **Bind UI to real subjects** - Replace mock data with printer_state subjects in XML
   - Home panel: connection state, temps, print status
   - Controls panels: real-time position, temperatures

3. 🔌 **Implement control actions** - Wire buttons to gcode_script() calls
   - Motion: jog commands, homing
   - Temps: SET_HEATER_TEMPERATURE commands
   - Extrusion: EXTRUDE/RETRACT commands

4. 📁 **File operations** - Get real print files from Moonraker
   - server.files.list integration
   - Print select panel with real data
   - Thumbnail extraction

---

## Critical Architecture Patterns

### Navigation System

Always use `ui_nav_push_overlay()` and `ui_nav_go_back()`:

```cpp
// Show overlay panel
ui_nav_push_overlay(motion_panel);

// Back button
ui_nav_go_back();  // Handles stack, shows previous or HOME
```

Nav bar buttons clear stack automatically. State preserved when navigating back.

**CRITICAL:** Never hide `app_layout` - prevents navbar disappearing.

### Subject Initialization Order

Subjects MUST be initialized BEFORE XML creation:

```cpp
// 1. Register XML components
lv_xml_component_register_from_file("A:/ui_xml/globals.xml");

// 2. Initialize subjects FIRST
ui_nav_init();
ui_panel_home_init_subjects();

// 3. NOW create UI
lv_obj_t* screen = lv_xml_create(NULL, "app_layout", NULL);
```

### Event Callbacks

Use `<lv_event-call_function>`, NOT `<event_cb>`:

```xml
<lv_button name="my_button">
    <lv_event-call_function trigger="clicked" callback="my_handler"/>
</lv_button>
```

Register in C++ before XML loads:
```cpp
lv_xml_register_event_cb(NULL, "my_handler", my_handler_function);
```

### Component Names

Always add explicit `name` attributes to component tags:

```xml
<lv_obj name="content_area">
  <controls_panel name="controls_panel"/>  <!-- Explicit name required -->
</lv_obj>
```

### Name-Based Widget Lookup

Always use names, never indices:

```cpp
// ✓ CORRECT
lv_obj_t* widget = lv_obj_find_by_name(parent, "widget_name");

// ✗ WRONG
lv_obj_t* widget = lv_obj_get_child(parent, 3);
```

---

## Next Priority: Moonraker Integration 🔌

**All UI complete. Ready to connect to live printer.**

### Step 1: WebSocket Foundation
- Review existing HelixScreen Moonraker client code (parent repo)
- Adapt libhv WebSocket implementation
- Connect on startup, handle connection events

### Step 2: Printer Status Updates
- Subscribe to printer object updates
- Wire temperature subjects to live data
- Update home panel with real-time temps

### Step 3: Motion & Control Commands
- Jog buttons → `printer.gcode.script` (G0/G1)
- Temperature presets → M104/M140 commands
- Home buttons → G28 commands

### Step 4: Print Management
- File list → `server.files.list` API
- Print start/pause/resume/cancel commands
- Live print status updates

**Existing subjects (already wired):**
- Print progress, layer, elapsed/remaining time
- Nozzle/bed temps, speed, flow
- Print state (Printing/Paused/Complete)

---

## Testing Commands

```bash
# Build
make                          # Incremental build (auto-parallel)
make clean && make            # Clean rebuild

# Run
./build/bin/helix-ui-proto                    # Default (medium, home panel)
./build/bin/helix-ui-proto -s tiny            # 480×320
./build/bin/helix-ui-proto -s large           # 1280×720
./build/bin/helix-ui-proto -p controls        # Start at Controls
./build/bin/helix-ui-proto -p print-select    # Print select

# Controls
# Cmd+Q (macOS) / Win+Q (Windows) to quit
# 'S' key to save screenshot

# Screenshot
./scripts/screenshot.sh helix-ui-proto output-name [panel-name]
```

**Screen sizes:** tiny (480×320), small (800×480), medium (1024×600), large (1280×720)

**Panel names:** home, controls, motion, nozzle-temp, bed-temp, extrusion, print-select, file-detail, filament, settings, advanced

---

## Documentation

- **[STATUS.md](STATUS.md)** - Complete chronological development journal
- **[ROADMAP.md](docs/ROADMAP.md)** - Planned features
- **[LVGL9_XML_GUIDE.md](docs/LVGL9_XML_GUIDE.md)** - LVGL 9 XML reference
- **[QUICK_REFERENCE.md](docs/QUICK_REFERENCE.md)** - Common patterns

---

## Known Gotchas

### LVGL 9 XML Attribute Names

**No `flag_` prefix:**
```xml
<!-- ✓ CORRECT -->
<lv_button hidden="true" clickable="false"/>

<!-- ✗ WRONG -->
<lv_button flag_hidden="true" flag_clickable="false"/>
```

**Use `style_image_*`, not `style_img_*`:**
```xml
<!-- ✓ CORRECT -->
<lv_image style_image_recolor="#primary_color" style_image_recolor_opa="255"/>

<!-- ✗ WRONG -->
<lv_image style_img_recolor="#primary_color" style_img_recolor_opa="255"/>
```

**Use `scale_x`/`scale_y`, not `zoom`:**
```xml
<!-- ✓ CORRECT (256 = 100%) -->
<lv_image scale_x="128" scale_y="128"/>

<!-- ✗ WRONG -->
<lv_image zoom="128"/>
```

### Subject Type Must Match API

Image recoloring requires color subjects:
```cpp
// ✓ CORRECT
lv_subject_init_color(&subject, lv_color_hex(0xFFD700));
lv_obj_set_style_img_recolor(widget, color, LV_PART_MAIN);

// ✗ WRONG
lv_subject_init_string(&subject, buffer, NULL, size, "0xFFD700");
```

---

**For complete development history, see STATUS.md**
