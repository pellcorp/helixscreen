<p align="center">
  <img src="assets/images/helix-icon-64.png" alt="HelixScreen" width="64"/>
</p>

# HelixScreen

[![Build](https://github.com/prestonbrown/guppyscreen/actions/workflows/prototype-ui9-build.yml/badge.svg?branch=ui-redesign)](https://github.com/prestonbrown/guppyscreen/actions/workflows/prototype-ui9-build.yml)
[![Code Quality](https://github.com/prestonbrown/guppyscreen/actions/workflows/prototype-ui9-quality.yml/badge.svg?branch=ui-redesign)](https://github.com/prestonbrown/guppyscreen/actions/workflows/prototype-ui9-quality.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![LVGL](https://img.shields.io/badge/LVGL-9.4.0-green.svg)](https://lvgl.io/)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey.svg)](#)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

**A modern, lightweight touch interface for Klipper/Moonraker 3D printers**

HelixScreen is a next-generation printer control interface built from the ground up using LVGL 9's declarative XML system. Designed for embedded hardware with limited resources, it brings advanced Klipper features to printers that ship with restrictive vendor UIs.

**Built on proven foundations:**
- Based on [GuppyScreen](https://github.com/ballaswag/guppyscreen) architecture and design patterns
- Potential integration with [KlipperScreen](https://github.com/KlipperScreen/KlipperScreen) features
- Modern XML-based UI with reactive data binding (LVGL 9.4)

**Key Goals:**
- 🚀 **More features** - Unlock Klipper's full potential beyond vendor limitations
- 💻 **Better hardware support** - Run on limited embedded devices (Pi, BTT Pad, vendor displays)
- 🎨 **Modern UI** - Clean, responsive touch interface with visual polish
- 📦 **Lightweight** - Minimal resource footprint for constrained hardware

## Target Hardware

HelixScreen is designed for embedded Linux devices with limited resources:

- **Raspberry Pi** (Pi 3/4/5, Zero 2 W)
- **BTT Pad 7** / similar touch displays
- **Vendor printer displays** (Creality K1/K1 Max, FlashForge AD5M, etc.)
- **Generic Linux ARM/x64** with framebuffer support

**Development simulator:** macOS/Linux desktop with SDL2 for rapid iteration

## Architecture Overview

HelixScreen uses a modern, declarative approach to embedded UI development:

- **Declarative XML layouts** - UI structure completely separate from application logic
- **Reactive data binding** - Automatic UI updates via Subject-Observer pattern
- **SDL2 simulator** - Rapid desktop development before embedded deployment
- **Global theme system** - Change colors/styles in a single file

**Key Innovation:** The entire UI is defined in XML files. C++ code only handles initialization and reactive data updates—zero layout or styling logic.

## Quick Start

### Prerequisites

**macOS:**
```bash
brew install sdl2 bear imagemagick python3 node
npm install  # Install lv_font_conv and lv_img_conv
```

**Debian/Ubuntu:**
```bash
sudo apt install libsdl2-dev bear imagemagick python3 clang make npm
npm install  # Install lv_font_conv and lv_img_conv
```

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install SDL2-devel bear ImageMagick python3 clang make npm
npm install  # Install lv_font_conv and lv_img_conv
```

**Dependencies:**
- **Required:** `clang`, `libsdl2-dev`/`SDL2-devel`, `make`, `python3`, `node`/`npm`
- **Font tools:** `lv_font_conv` (installed via `npm install`)
- **Optional:** `bear` (IDE/LSP support), `imagemagick` (screenshots)

**Auto-install:**
```bash
# Check what's missing
make check-deps

# Automatically install missing dependencies (interactive)
make install-deps
```

### Build & Run

```bash
# Build (auto-parallel, uses all CPU cores)
make

# Run simulator
./build/bin/helix-ui-proto

# Generate IDE support (one-time setup)
make compile_commands

# Clean rebuild
make clean && make
```

**Controls:**
- Click navigation icons to switch panels
- Press **S** to save screenshot
- Close window to exit

## Features

### Declarative XML UI System

Complete UI defined in XML files without touching C++ for layout or styling:

```xml
<!-- Define a panel in XML -->
<component>
  <view extends="lv_obj" style_bg_color="#bg_dark" style_pad_all="20">
    <lv_label text="Nozzle Temperature" style_text_color="#text_primary"/>
    <lv_label bind_text="temp_text" style_text_font="montserrat_28"/>
  </view>
</component>
```

```cpp
// C++ is pure logic - zero layout code
ui_panel_nozzle_init_subjects();
lv_xml_create(screen, "nozzle_panel", NULL);
ui_panel_nozzle_update(210);  // All bound widgets update automatically
```

### Reactive Data Binding

LVGL 9's Subject-Observer pattern enables automatic UI updates:

- **No manual widget searching** - XML bindings handle everything
- **Type-safe updates** - C++ API prevents runtime errors
- **One update, multiple widgets** - All bound elements react instantly
- **Clean separation** - UI structure and business logic are independent

### Global Theme System

Define colors, sizes, and constants in one place (`ui_xml/globals.xml`):

```xml
<consts>
  <color name="primary_color" value="0xff4444"/>
  <color name="bg_dark" value="0x1a1a1a"/>
  <px name="nav_width" value="102"/>
</consts>
```

Reference with `#name` syntax throughout all XML files. Change theme globally by editing one file.

### Hybrid Icon System

Mix FontAwesome fonts and custom PNG icons with unified reactive recoloring:

- **FontAwesome 6** icons auto-generated from C++ constants
- **Custom SVG icons** converted to PNG with ImageMagick
- **Reactive colors** - icons change color on selection automatically
- **Type-agnostic** - system handles fonts and images identically

### Platform-Independent Screenshots

- Press **S** during runtime for timestamped screenshots
- Uses LVGL's native `lv_snapshot_take()` API
- Works on any display backend (SDL, framebuffer, DRM)
- Automated screenshot script for CI/testing

## Architecture

```
XML Layout (ui_xml/*.xml)
    ↓ bind_text / bind_value / bind_flag
Reactive Subjects (lv_subject_t)
    ↓ lv_subject_set_* / copy_*
C++ Application Logic (src/*.cpp)
```

### Component Hierarchy

```
app_layout.xml
├── navigation_bar.xml      # 5-button vertical navigation
└── content_area
    ├── home_panel.xml       # Print status overview
    ├── controls_panel.xml   # Motion/temperature/extrusion launcher
    │   ├── motion_panel.xml
    │   ├── nozzle_temp_panel.xml
    │   ├── bed_temp_panel.xml
    │   └── extrusion_panel.xml
    ├── print_select_panel.xml
    ├── filament_panel.xml
    ├── settings_panel.xml
    └── advanced_panel.xml
```

All components reference `globals.xml` for shared constants.

## Project Structure

```
prototype-ui9/
├── src/                    # C++ application logic
│   ├── main.cpp            # Entry point, subject initialization
│   ├── ui_nav.cpp          # Navigation state management
│   ├── ui_panel_*.cpp      # Panel-specific reactive bindings
│   └── ui_*.cpp            # Shared utilities
├── include/                # C++ headers
│   ├── ui_fonts.h          # FontAwesome icon constants
│   └── ui_*.h              # Panel APIs and utilities
├── ui_xml/                 # Declarative UI layouts
│   ├── globals.xml         # Theme constants (colors, sizes, icons)
│   ├── app_layout.xml      # Root layout structure
│   └── *_panel.xml         # Individual panel definitions
├── assets/
│   ├── fonts/              # FontAwesome 6 font files
│   └── images/             # UI icons and graphics
├── docs/                   # Detailed documentation
│   ├── LVGL9_XML_GUIDE.md  # Complete XML system reference
│   ├── QUICK_REFERENCE.md  # Common patterns and gotchas
│   └── COPYRIGHT_HEADERS.md
└── scripts/
    ├── generate-icon-consts.py  # Auto-generate icon constants
    └── screenshot.sh            # Automated screenshot tool
```

## Development Workflow

1. **Edit XML** for layout/styling changes (no recompilation needed)
2. **Edit C++** for logic/subjects changes → `make`
3. **Test** with `./build/bin/helix-ui-proto [panel_name]`
4. **Screenshot** with `./scripts/screenshot.sh` or press **S** in UI
5. **Commit** with working incremental changes

### Icon Generation

FontAwesome icons are auto-generated to avoid UTF-8 encoding issues:

```bash
# After editing icon definitions in include/ui_fonts.h
python3 scripts/generate-icon-consts.py
```

This updates `ui_xml/globals.xml` with UTF-8 byte sequences for all icons.

### Multi-Display Support (macOS)

Control which display the UI window appears on:

```bash
# Center window on specific display
./build/bin/helix-ui-proto --display 0     # Display 0 (main)
./build/bin/helix-ui-proto --display 1     # Display 1 (secondary)

# Position at exact coordinates
./build/bin/helix-ui-proto --x-pos 100 --y-pos 200

# Combine with other options
./build/bin/helix-ui-proto -d 1 -s small --panel home
```

**How it works:**
- Uses `SDL_GetDisplayBounds()` to query actual display geometry
- Calculates true center position for the specified display
- Supports multi-monitor setups with different resolutions
- Useful for dual-display development workflows

**Available displays:**
Run without arguments to see auto-detected display information in logs.

### Screenshot Workflow

The screenshot utility provides automated build → run → capture → convert workflow:

```bash
# Interactive: press S while running
./build/bin/helix-ui-proto

# Automated: build + run + screenshot (auto-opens on display 1)
./scripts/screenshot.sh helix-ui-proto output-name [panel] [options]

# Examples
./scripts/screenshot.sh helix-ui-proto home-screen home
./scripts/screenshot.sh helix-ui-proto motion-panel motion -s small
./scripts/screenshot.sh helix-ui-proto controls controls -s large

# Override display for screenshots
HELIX_SCREENSHOT_DISPLAY=0 ./scripts/screenshot.sh helix-ui-proto test home

# Auto-open in Preview after capture
HELIX_SCREENSHOT_OPEN=1 ./scripts/screenshot.sh helix-ui-proto review home
```

**Features:**
- ✅ **Automatic display positioning** - Opens on display 1 by default (keeps terminal visible)
- ✅ **Colored output** - Visual progress indicators and status messages
- ✅ **Panel validation** - Catches invalid panel names before running
- ✅ **Dependency checking** - Verifies gtimeout and ImageMagick are installed
- ✅ **Error handling** - Clear error messages with troubleshooting hints
- ✅ **Smart cleanup** - Auto-removes BMP, keeps only compressed PNG
- ✅ **Optional preview** - Set `HELIX_SCREENSHOT_OPEN=1` to auto-open result

Screenshots saved to `/tmp/ui-screenshot-[name].png`

## Key Technical Details

### Subject Initialization Order

**Critical:** Initialize subjects BEFORE creating XML:

```cpp
// 1. Register XML components
lv_xml_component_register_from_file("A:/ui_xml/globals.xml");
lv_xml_component_register_from_file("A:/ui_xml/home_panel.xml");

// 2. Initialize subjects
ui_nav_init();
ui_panel_home_init_subjects();

// 3. NOW create UI
lv_xml_create(screen, "app_layout", NULL);
```

If subjects are created in XML before C++ initialization, they'll have empty/default values.

### Event Loop Integration

**Important:** LVGL's SDL driver handles all event polling internally. Never call `SDL_PollEvent()` manually:

```cpp
// ✓ CORRECT
while (lv_display_get_next(NULL)) {
    lv_timer_handler();  // Internally polls SDL events
    SDL_Delay(5);
}

// ✗ WRONG - breaks click events
while (running) {
    SDL_Event event;
    SDL_PollEvent(&event);  // Drains queue before LVGL sees it
    lv_timer_handler();
}
```

This applies to ALL display drivers (SDL, framebuffer, DRM).

### LVGL Configuration

Key settings in `lv_conf.h`:
- `LV_USE_XML 1` - Enable XML UI support
- `LV_USE_SNAPSHOT 1` - Enable screenshot API
- `LV_USE_DRAW_SW_COMPLEX_GRADIENTS 1` - Required by XML parser
- `LV_FONT_MONTSERRAT_16/20/28 1` - Text fonts

## Documentation

- **[LVGL 9 XML Guide](docs/LVGL9_XML_GUIDE.md)** - Complete XML system reference with troubleshooting
- **[Quick Reference](docs/QUICK_REFERENCE.md)** - Common patterns and code snippets
- **[CLAUDE.md](CLAUDE.md)** - Project context for AI assistants
- **[HANDOFF.md](HANDOFF.md)** - Current work status and priorities
- **[STATUS.md](STATUS.md)** - Documentation guide and architectural decisions
- **[ROADMAP.md](docs/ROADMAP.md)** - Planned features and milestones

## Project Status

See **[HANDOFF.md](HANDOFF.md)** for current work status and next priorities.

## Notes & Gotchas

- **XML support is experimental** in LVGL 9.3.0
- Uses POSIX filesystem with **'A:' drive letter** for file paths
- All XML files must be **UTF-8 encoded**
- Flex layouts: use `style_flex_main_place`/`style_flex_cross_place` (not deprecated `flex_align`)
- Component names come from **filenames**, not `<view name="...">` attributes
- Always add explicit `name="..."` attributes when instantiating components in XML

## License

GPL v3 - See individual source files for copyright headers.

## Related Projects & Acknowledgments

**HelixScreen builds upon:**
- **[GuppyScreen](https://github.com/ballaswag/guppyscreen)** - Core architecture and Moonraker integration patterns
- **[KlipperScreen](https://github.com/KlipperScreen/KlipperScreen)** - Feature inspiration and UI concepts

**Technology Stack:**
- **[LVGL 9.4](https://lvgl.io/)** - Light and Versatile Graphics Library
- **[Klipper](https://www.klipper3d.org/)** - Advanced 3D printer firmware
- **[Moonraker](https://github.com/Arksine/moonraker)** - Klipper API server
- **[libhv](https://github.com/ithewei/libhv)** v1.3.4 - Async networking (WebSocket, HTTP)
- **[wpa_supplicant](https://w1.fi/wpa_supplicant/)** v2.11 - WiFi control (Linux only)
- **[spdlog](https://github.com/gabime/spdlog)** - Fast C++ logging library
