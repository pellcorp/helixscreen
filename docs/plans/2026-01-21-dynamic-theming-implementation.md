# Dynamic Theming System Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Replace hardcoded Nord colors with a dynamic JSON-based theming system supporting custom palettes, live preview, and a theme editor UI.

**Architecture:** Theme files (JSON) define 16 palette colors + 4 style properties. At startup, `ThemeLoader` reads the active theme and registers colors as LVGL constants. A hardcoded C++ mapping converts palette colors to semantic colors (app_bg_color, text_primary, etc.) with _light/_dark variants. Live preview bypasses the constant system by directly updating helix_theme styles.

**Tech Stack:** C++17, LVGL 9.4, nlohmann/json (via libhv), existing ui_theme/helix_theme infrastructure.

**Design Doc:** `docs/plans/2026-01-21-dynamic-theming-design.md`

---

## Phase 1: Theme Loader Core

Create the `ThemeLoader` module with data structures, JSON parsing, and file operations.

### Task 1.1: Create ThemeData Structure

**Files:**
- Create: `include/theme_loader.h`

**Step 1: Write the header file**

```cpp
// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string>
#include <vector>

namespace helix {

/**
 * @brief 16-color palette with semantic names
 *
 * Indices map to palette slots defined in design doc:
 * 0-3: Dark backgrounds, 4-6: Light backgrounds,
 * 7-10: Accents, 11-15: Status colors
 */
struct ThemePalette {
    std::string bg_darkest;       // 0: Dark mode app background
    std::string bg_dark;          // 1: Dark mode cards/surfaces
    std::string bg_dark_highlight;// 2: Selection highlight on dark
    std::string border_muted;     // 3: Borders, muted text
    std::string text_light;       // 4: Primary text on dark surfaces
    std::string bg_light;         // 5: Light mode cards/surfaces
    std::string bg_lightest;      // 6: Light mode app background
    std::string accent_highlight; // 7: Subtle highlights
    std::string accent_primary;   // 8: Primary accent, links
    std::string accent_secondary; // 9: Secondary accent
    std::string accent_tertiary;  // 10: Tertiary accent
    std::string status_error;     // 11: Error, danger (red)
    std::string status_danger;    // 12: Danger, attention (orange)
    std::string status_warning;   // 13: Warning, caution (yellow)
    std::string status_success;   // 14: Success, positive (green)
    std::string status_special;   // 15: Special, unusual (purple)

    /** @brief Access color by index (0-15) */
    const std::string& at(size_t index) const;
    std::string& at(size_t index);

    /** @brief Get array of all color names for iteration */
    static const std::array<const char*, 16>& color_names();
};

/**
 * @brief Non-color theme properties
 */
struct ThemeProperties {
    int border_radius = 12;    // Corner roundness (0 = sharp, 12 = soft)
    int border_width = 1;      // Default border width
    int border_opacity = 40;   // Border opacity (0-255)
    int shadow_intensity = 0;  // Shadow strength (0 = disabled)
};

/**
 * @brief Complete theme definition
 */
struct ThemeData {
    std::string name;          // Display name (shown in UI)
    std::string filename;      // Source filename (without .json)
    ThemePalette colors;
    ThemeProperties properties;

    /** @brief Check if theme has all required colors */
    bool is_valid() const;
};

/**
 * @brief Theme file info for discovery listing
 */
struct ThemeInfo {
    std::string filename;      // e.g., "nord"
    std::string display_name;  // e.g., "Nord"
};

} // namespace helix
```

**Step 2: Commit**

```bash
git add include/theme_loader.h
git commit -m "feat(theme): add ThemeData structures for dynamic theming"
```

### Task 1.2: Implement ThemePalette Accessors

**Files:**
- Create: `src/ui/theme_loader.cpp`
- Test: `tests/unit/test_theme_loader.cpp`

**Step 1: Write the failing test**

```cpp
// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "theme_loader.h"

#include <catch2/catch_test_macros.hpp>

using namespace helix;

TEST_CASE("ThemePalette index access", "[theme]") {
    ThemePalette palette;
    palette.bg_darkest = "#2e3440";
    palette.status_special = "#b48ead";

    REQUIRE(palette.at(0) == "#2e3440");
    REQUIRE(palette.at(15) == "#b48ead");
}

TEST_CASE("ThemePalette color_names returns all 16 names", "[theme]") {
    auto& names = ThemePalette::color_names();
    REQUIRE(names.size() == 16);
    REQUIRE(std::string(names[0]) == "bg_darkest");
    REQUIRE(std::string(names[15]) == "status_special");
}
```

**Step 2: Run test to verify it fails**

Run: `./build/bin/helix-tests "[theme]" -v`
Expected: FAIL - theme_loader.cpp not found

**Step 3: Write the implementation**

```cpp
// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "theme_loader.h"

#include <spdlog/spdlog.h>
#include <stdexcept>

namespace helix {

const std::array<const char*, 16>& ThemePalette::color_names() {
    static const std::array<const char*, 16> names = {
        "bg_darkest", "bg_dark", "bg_dark_highlight", "border_muted",
        "text_light", "bg_light", "bg_lightest", "accent_highlight",
        "accent_primary", "accent_secondary", "accent_tertiary",
        "status_error", "status_danger", "status_warning",
        "status_success", "status_special"
    };
    return names;
}

const std::string& ThemePalette::at(size_t index) const {
    switch (index) {
        case 0: return bg_darkest;
        case 1: return bg_dark;
        case 2: return bg_dark_highlight;
        case 3: return border_muted;
        case 4: return text_light;
        case 5: return bg_light;
        case 6: return bg_lightest;
        case 7: return accent_highlight;
        case 8: return accent_primary;
        case 9: return accent_secondary;
        case 10: return accent_tertiary;
        case 11: return status_error;
        case 12: return status_danger;
        case 13: return status_warning;
        case 14: return status_success;
        case 15: return status_special;
        default: throw std::out_of_range("ThemePalette index out of range");
    }
}

std::string& ThemePalette::at(size_t index) {
    return const_cast<std::string&>(
        static_cast<const ThemePalette*>(this)->at(index));
}

bool ThemeData::is_valid() const {
    // Check all colors are non-empty and start with #
    for (size_t i = 0; i < 16; ++i) {
        const auto& color = colors.at(i);
        if (color.empty() || color[0] != '#' || color.size() != 7) {
            return false;
        }
    }
    return !name.empty();
}

} // namespace helix
```

**Step 4: Update Makefile to include theme_loader.cpp**

Add to `SRC_FILES` in Makefile (find the ui sources section):
```makefile
src/ui/theme_loader.cpp \
```

**Step 5: Run test to verify it passes**

Run: `make -j && ./build/bin/helix-tests "[theme]" -v`
Expected: PASS

**Step 6: Commit**

```bash
git add src/ui/theme_loader.cpp tests/unit/test_theme_loader.cpp Makefile
git commit -m "feat(theme): implement ThemePalette accessors with tests"
```

### Task 1.3: Implement JSON Parsing

**Files:**
- Modify: `src/ui/theme_loader.cpp`
- Modify: `include/theme_loader.h`
- Modify: `tests/unit/test_theme_loader.cpp`

**Step 1: Add function declarations to header**

Add to `include/theme_loader.h` before closing namespace:

```cpp
/**
 * @brief Load theme from JSON file
 * @param filepath Full path to theme JSON file
 * @return ThemeData with parsed values, or empty ThemeData on error
 */
ThemeData load_theme_from_file(const std::string& filepath);

/**
 * @brief Parse theme from JSON string
 * @param json_str JSON content
 * @param filename Source filename for error messages
 * @return ThemeData with parsed values
 */
ThemeData parse_theme_json(const std::string& json_str, const std::string& filename);

/**
 * @brief Save theme to JSON file
 * @param theme Theme data to save
 * @param filepath Full path to output file
 * @return true on success, false on error
 */
bool save_theme_to_file(const ThemeData& theme, const std::string& filepath);

/**
 * @brief Get default Nord theme (fallback)
 * @return ThemeData with Nord color values
 */
ThemeData get_default_nord_theme();
```

**Step 2: Write the failing test**

Add to `tests/unit/test_theme_loader.cpp`:

```cpp
TEST_CASE("parse_theme_json parses valid theme", "[theme]") {
    const char* json = R"({
        "name": "Test Theme",
        "colors": {
            "bg_darkest": "#2e3440",
            "bg_dark": "#3b4252",
            "bg_dark_highlight": "#434c5e",
            "border_muted": "#4c566a",
            "text_light": "#d8dee9",
            "bg_light": "#e5e9f0",
            "bg_lightest": "#eceff4",
            "accent_highlight": "#8fbcbb",
            "accent_primary": "#88c0d0",
            "accent_secondary": "#81a1c1",
            "accent_tertiary": "#5e81ac",
            "status_error": "#bf616a",
            "status_danger": "#d08770",
            "status_warning": "#ebcb8b",
            "status_success": "#a3be8c",
            "status_special": "#b48ead"
        },
        "border_radius": 8,
        "border_width": 2,
        "border_opacity": 50,
        "shadow_intensity": 10
    })";

    auto theme = helix::parse_theme_json(json, "test.json");

    REQUIRE(theme.name == "Test Theme");
    REQUIRE(theme.colors.bg_darkest == "#2e3440");
    REQUIRE(theme.colors.status_special == "#b48ead");
    REQUIRE(theme.properties.border_radius == 8);
    REQUIRE(theme.properties.shadow_intensity == 10);
    REQUIRE(theme.is_valid());
}

TEST_CASE("get_default_nord_theme returns valid theme", "[theme]") {
    auto theme = helix::get_default_nord_theme();

    REQUIRE(theme.name == "Nord");
    REQUIRE(theme.is_valid());
    REQUIRE(theme.colors.bg_darkest == "#2e3440");
}
```

**Step 3: Run test to verify it fails**

Run: `make -j && ./build/bin/helix-tests "[theme]" -v`
Expected: FAIL - parse_theme_json not defined

**Step 4: Write the implementation**

Add to `src/ui/theme_loader.cpp`:

```cpp
#include "hv/json.hpp"

#include <fstream>
#include <sstream>

namespace helix {

ThemeData get_default_nord_theme() {
    ThemeData theme;
    theme.name = "Nord";
    theme.filename = "nord";

    theme.colors.bg_darkest = "#2e3440";
    theme.colors.bg_dark = "#3b4252";
    theme.colors.bg_dark_highlight = "#434c5e";
    theme.colors.border_muted = "#4c566a";
    theme.colors.text_light = "#d8dee9";
    theme.colors.bg_light = "#e5e9f0";
    theme.colors.bg_lightest = "#eceff4";
    theme.colors.accent_highlight = "#8fbcbb";
    theme.colors.accent_primary = "#88c0d0";
    theme.colors.accent_secondary = "#81a1c1";
    theme.colors.accent_tertiary = "#5e81ac";
    theme.colors.status_error = "#bf616a";
    theme.colors.status_danger = "#d08770";
    theme.colors.status_warning = "#ebcb8b";
    theme.colors.status_success = "#a3be8c";
    theme.colors.status_special = "#b48ead";

    theme.properties.border_radius = 12;
    theme.properties.border_width = 1;
    theme.properties.border_opacity = 40;
    theme.properties.shadow_intensity = 0;

    return theme;
}

ThemeData parse_theme_json(const std::string& json_str, const std::string& filename) {
    ThemeData theme;
    theme.filename = filename;

    // Remove .json extension if present
    if (theme.filename.size() > 5 &&
        theme.filename.substr(theme.filename.size() - 5) == ".json") {
        theme.filename = theme.filename.substr(0, theme.filename.size() - 5);
    }

    try {
        auto json = nlohmann::json::parse(json_str);

        theme.name = json.value("name", "Unnamed Theme");

        // Parse colors
        if (json.contains("colors")) {
            auto& colors = json["colors"];
            auto& names = ThemePalette::color_names();
            auto defaults = get_default_nord_theme();

            for (size_t i = 0; i < 16; ++i) {
                const char* name = names[i];
                if (colors.contains(name)) {
                    theme.colors.at(i) = colors[name].get<std::string>();
                } else {
                    // Fall back to Nord default
                    theme.colors.at(i) = defaults.colors.at(i);
                    spdlog::warn("[ThemeLoader] Missing color '{}' in {}, using Nord default",
                                 name, filename);
                }
            }
        } else {
            spdlog::error("[ThemeLoader] No 'colors' object in {}", filename);
            return get_default_nord_theme();
        }

        // Parse properties with defaults
        theme.properties.border_radius = json.value("border_radius", 12);
        theme.properties.border_width = json.value("border_width", 1);
        theme.properties.border_opacity = json.value("border_opacity", 40);
        theme.properties.shadow_intensity = json.value("shadow_intensity", 0);

    } catch (const nlohmann::json::exception& e) {
        spdlog::error("[ThemeLoader] Failed to parse {}: {}", filename, e.what());
        return get_default_nord_theme();
    }

    return theme;
}

ThemeData load_theme_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("[ThemeLoader] Failed to open {}", filepath);
        return {};
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    // Extract filename from path
    std::string filename = filepath;
    size_t slash = filepath.rfind('/');
    if (slash != std::string::npos) {
        filename = filepath.substr(slash + 1);
    }

    return parse_theme_json(buffer.str(), filename);
}

bool save_theme_to_file(const ThemeData& theme, const std::string& filepath) {
    nlohmann::json json;

    json["name"] = theme.name;

    // Build colors object
    nlohmann::json colors;
    auto& names = ThemePalette::color_names();
    for (size_t i = 0; i < 16; ++i) {
        colors[names[i]] = theme.colors.at(i);
    }
    json["colors"] = colors;

    // Properties
    json["border_radius"] = theme.properties.border_radius;
    json["border_width"] = theme.properties.border_width;
    json["border_opacity"] = theme.properties.border_opacity;
    json["shadow_intensity"] = theme.properties.shadow_intensity;

    // Write with pretty formatting
    std::ofstream file(filepath);
    if (!file.is_open()) {
        spdlog::error("[ThemeLoader] Failed to write {}", filepath);
        return false;
    }

    file << json.dump(2);
    return true;
}

} // namespace helix
```

**Step 5: Run test to verify it passes**

Run: `make -j && ./build/bin/helix-tests "[theme]" -v`
Expected: PASS

**Step 6: Commit**

```bash
git add include/theme_loader.h src/ui/theme_loader.cpp tests/unit/test_theme_loader.cpp
git commit -m "feat(theme): implement JSON parsing and serialization"
```

### Task 1.4: Implement Theme Discovery

**Files:**
- Modify: `include/theme_loader.h`
- Modify: `src/ui/theme_loader.cpp`
- Modify: `tests/unit/test_theme_loader.cpp`

**Step 1: Add function declarations to header**

Add to `include/theme_loader.h`:

```cpp
/**
 * @brief Discover all theme files in directory
 * @param themes_dir Path to themes directory
 * @return List of theme info (filename, display_name)
 */
std::vector<ThemeInfo> discover_themes(const std::string& themes_dir);

/**
 * @brief Ensure themes directory exists with default theme
 * @param themes_dir Path to themes directory
 * @return true if directory is ready, false on error
 */
bool ensure_themes_directory(const std::string& themes_dir);

/**
 * @brief Get themes directory path
 * @return Full path to $HELIXDIR/config/themes/
 */
std::string get_themes_directory();
```

**Step 2: Write the implementation**

Add to `src/ui/theme_loader.cpp`:

```cpp
#include "app_globals.h"  // For get_helix_dir()

#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

namespace helix {

std::string get_themes_directory() {
    return std::string(get_helix_dir()) + "/config/themes";
}

bool ensure_themes_directory(const std::string& themes_dir) {
    // Create directory if it doesn't exist
    struct stat st;
    if (stat(themes_dir.c_str(), &st) != 0) {
        // Directory doesn't exist, create it
        if (mkdir(themes_dir.c_str(), 0755) != 0) {
            spdlog::error("[ThemeLoader] Failed to create themes directory: {}", themes_dir);
            return false;
        }
        spdlog::info("[ThemeLoader] Created themes directory: {}", themes_dir);
    }

    // Check if nord.json exists, create if missing
    std::string nord_path = themes_dir + "/nord.json";
    if (stat(nord_path.c_str(), &st) != 0) {
        auto nord = get_default_nord_theme();
        if (!save_theme_to_file(nord, nord_path)) {
            spdlog::error("[ThemeLoader] Failed to create default nord.json");
            return false;
        }
        spdlog::info("[ThemeLoader] Created default theme: {}", nord_path);
    }

    return true;
}

std::vector<ThemeInfo> discover_themes(const std::string& themes_dir) {
    std::vector<ThemeInfo> themes;

    DIR* dir = opendir(themes_dir.c_str());
    if (!dir) {
        spdlog::warn("[ThemeLoader] Could not open themes directory: {}", themes_dir);
        return themes;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;

        // Skip non-json files
        if (filename.size() <= 5 ||
            filename.substr(filename.size() - 5) != ".json") {
            continue;
        }

        // Skip hidden files
        if (filename[0] == '.') {
            continue;
        }

        std::string filepath = themes_dir + "/" + filename;
        auto theme = load_theme_from_file(filepath);

        if (theme.is_valid()) {
            ThemeInfo info;
            info.filename = filename.substr(0, filename.size() - 5);  // Remove .json
            info.display_name = theme.name;
            themes.push_back(info);
        }
    }

    closedir(dir);

    // Sort alphabetically by display name
    std::sort(themes.begin(), themes.end(),
              [](const ThemeInfo& a, const ThemeInfo& b) {
                  return a.display_name < b.display_name;
              });

    spdlog::debug("[ThemeLoader] Discovered {} themes in {}", themes.size(), themes_dir);
    return themes;
}

} // namespace helix
```

**Step 3: Commit**

```bash
git add include/theme_loader.h src/ui/theme_loader.cpp
git commit -m "feat(theme): implement theme discovery and directory setup"
```

---

## Phase 2: Theme Loading Integration

Hook ThemeLoader into the existing ui_theme initialization flow.

### Task 2.1: Add Theme Loading to ui_theme_init

**Files:**
- Modify: `src/ui/ui_theme.cpp`
- Modify: `include/ui_theme.h`

**Step 1: Add includes and active theme storage**

At top of `src/ui/ui_theme.cpp`, add:

```cpp
#include "theme_loader.h"

static helix::ThemeData active_theme;
```

**Step 2: Create helper to register palette colors as LVGL constants**

Add new function in `src/ui/ui_theme.cpp`:

```cpp
/**
 * @brief Register theme palette colors as LVGL constants
 *
 * Registers all 16 palette colors from the active theme.
 * Must be called BEFORE ui_theme_register_static_constants() so
 * palette colors are available for semantic mapping.
 */
static void ui_theme_register_palette_colors(lv_xml_component_scope_t* scope,
                                              const helix::ThemeData& theme) {
    auto& names = helix::ThemePalette::color_names();
    for (size_t i = 0; i < 16; ++i) {
        lv_xml_register_const(scope, names[i], theme.colors.at(i).c_str());
    }
    spdlog::debug("[Theme] Registered 16 palette colors from theme '{}'", theme.name);
}
```

**Step 3: Create function to load theme at startup**

Add new function:

```cpp
/**
 * @brief Load active theme from config
 *
 * Reads /display/theme from config, loads corresponding JSON file.
 * Falls back to Nord if not found.
 */
static helix::ThemeData ui_theme_load_active_theme() {
    std::string themes_dir = helix::get_themes_directory();

    // Ensure themes directory exists with default theme
    helix::ensure_themes_directory(themes_dir);

    // Read theme name from config
    Config* config = Config::get_instance();
    std::string theme_name = config ? config->get<std::string>("/display/theme", "nord") : "nord";

    // Load theme file
    std::string theme_path = themes_dir + "/" + theme_name + ".json";
    auto theme = helix::load_theme_from_file(theme_path);

    if (!theme.is_valid()) {
        spdlog::warn("[Theme] Theme '{}' not found or invalid, using Nord", theme_name);
        theme = helix::get_default_nord_theme();
    }

    spdlog::info("[Theme] Loaded theme: {} ({})", theme.name, theme.filename);
    return theme;
}
```

**Step 4: Modify ui_theme_init to use ThemeLoader**

In `ui_theme_init()`, after getting the globals scope, add:

```cpp
    // Load active theme from config/themes directory
    active_theme = ui_theme_load_active_theme();

    // Register palette colors FIRST (before static constants parse nord0-15 from XML)
    ui_theme_register_palette_colors(scope, active_theme);
```

**Step 5: Add getter for active theme**

Add to `include/ui_theme.h`:

```cpp
/**
 * @brief Get currently active theme data
 * @return Reference to active theme (valid after ui_theme_init)
 */
const helix::ThemeData& ui_theme_get_active_theme();
```

Add to `src/ui/ui_theme.cpp`:

```cpp
const helix::ThemeData& ui_theme_get_active_theme() {
    return active_theme;
}
```

**Step 6: Build and test manually**

Run: `make -j && ./build/bin/helix-screen --test -vv`
Expected: Should see "[Theme] Loaded theme: Nord (nord)" in logs

**Step 7: Commit**

```bash
git add src/ui/ui_theme.cpp include/ui_theme.h
git commit -m "feat(theme): integrate ThemeLoader into ui_theme_init"
```

### Task 2.2: Remove Hardcoded nord0-15 from globals.xml

**Files:**
- Modify: `ui_xml/globals.xml`

**Step 1: Remove nord palette definitions**

In `ui_xml/globals.xml`, find and remove these lines (around line 359-375):

```xml
    <!-- Nord palette tokens (used for swatches + accents) -->
    <color name="nord0" value="#2e3440"/>
    <color name="nord1" value="#3b4252"/>
    ... (all nord0 through nord15)
```

Keep the comment block explaining Nord colors (lines 330-356) for documentation.

**Step 2: Update theme_settings_overlay.xml to use new palette names**

In `ui_xml/theme_settings_overlay.xml`, replace all references:
- `#nord0` → `#bg_darkest`
- `#nord1` → `#bg_dark`
- `#nord2` → `#bg_dark_highlight`
- `#nord3` → `#border_muted`
- `#nord4` → `#text_light`
- `#nord5` → `#bg_light`
- `#nord6` → `#bg_lightest`
- `#nord7` → `#accent_highlight`
- `#nord8` → `#accent_primary`
- `#nord9` → `#accent_secondary`
- `#nord10` → `#accent_tertiary`
- `#nord11` → `#status_error`
- `#nord12` → `#status_danger`
- `#nord13` → `#status_warning`
- `#nord14` → `#status_success`
- `#nord15` → `#status_special`

**Step 3: Build and test**

Run: `make -j && ./build/bin/helix-screen --test -vv`
Expected: UI should look identical (palette colors now come from theme loader)

**Step 4: Commit**

```bash
git add ui_xml/globals.xml ui_xml/theme_settings_overlay.xml
git commit -m "refactor(theme): replace nord0-15 with palette color names"
```

---

## Phase 3: Semantic Color Mapping

Implement the mapping from palette colors to semantic colors with _light/_dark variants.

### Task 3.1: Create Semantic Mapping Function

**Files:**
- Modify: `src/ui/ui_theme.cpp`

**Step 1: Add semantic color mapping function**

Add new function:

```cpp
/**
 * @brief Register semantic colors derived from palette
 *
 * Maps palette colors to semantic colors (app_bg_color, text_primary, etc.)
 * with _light and _dark variants for theme mode switching.
 */
static void ui_theme_register_semantic_colors(lv_xml_component_scope_t* scope,
                                               const helix::ThemeData& theme) {
    const auto& c = theme.colors;

    // Background colors (mode-dependent)
    lv_xml_register_const(scope, "app_bg_color_dark", c.bg_darkest.c_str());
    lv_xml_register_const(scope, "app_bg_color_light", c.bg_lightest.c_str());

    lv_xml_register_const(scope, "card_bg_dark", c.bg_dark.c_str());
    lv_xml_register_const(scope, "card_bg_light", c.bg_light.c_str());

    lv_xml_register_const(scope, "selection_highlight_dark", c.bg_dark_highlight.c_str());
    lv_xml_register_const(scope, "selection_highlight_light", c.text_light.c_str());

    // Text colors (mode-dependent)
    lv_xml_register_const(scope, "text_primary_dark", c.bg_lightest.c_str());
    lv_xml_register_const(scope, "text_primary_light", c.bg_darkest.c_str());

    lv_xml_register_const(scope, "text_secondary_dark", c.text_light.c_str());
    lv_xml_register_const(scope, "text_secondary_light", c.border_muted.c_str());

    lv_xml_register_const(scope, "header_text_dark", c.bg_light.c_str());
    lv_xml_register_const(scope, "header_text_light", c.bg_dark.c_str());

    // Border/muted (mode-dependent)
    lv_xml_register_const(scope, "theme_grey_dark", c.border_muted.c_str());
    lv_xml_register_const(scope, "theme_grey_light", c.text_light.c_str());

    // Accent colors (same in both modes)
    lv_xml_register_const(scope, "primary_color", c.accent_primary.c_str());
    lv_xml_register_const(scope, "secondary_color", c.accent_secondary.c_str());
    lv_xml_register_const(scope, "tertiary_color", c.accent_tertiary.c_str());
    lv_xml_register_const(scope, "highlight_color", c.accent_highlight.c_str());

    // Status colors (same in both modes)
    lv_xml_register_const(scope, "error_color", c.status_error.c_str());
    lv_xml_register_const(scope, "danger_color", c.status_danger.c_str());
    lv_xml_register_const(scope, "attention_color", c.status_warning.c_str());
    lv_xml_register_const(scope, "success_color", c.status_success.c_str());
    lv_xml_register_const(scope, "special_color", c.status_special.c_str());
    lv_xml_register_const(scope, "info_color", c.accent_primary.c_str());

    // Keyboard colors (mode-dependent)
    lv_xml_register_const(scope, "keyboard_key_dark", c.bg_dark_highlight.c_str());
    lv_xml_register_const(scope, "keyboard_key_light", c.bg_lightest.c_str());
    lv_xml_register_const(scope, "keyboard_key_special_dark", c.bg_dark.c_str());
    lv_xml_register_const(scope, "keyboard_key_special_light", c.text_light.c_str());

    spdlog::debug("[Theme] Registered semantic colors from palette");
}
```

**Step 2: Call from ui_theme_init**

In `ui_theme_init()`, after `ui_theme_register_palette_colors()`:

```cpp
    // Register semantic colors derived from palette
    ui_theme_register_semantic_colors(scope, active_theme);
```

**Step 3: Remove redundant color definitions from globals.xml**

Remove these from `ui_xml/globals.xml` since they're now registered programmatically:
- `primary_color`, `secondary_color`, `tertiary_color`
- `error_color`, `warning_color`, `success_color`, `info_color`
- `app_bg_color_light`, `app_bg_color_dark`
- `text_primary_light`, `text_primary_dark`
- `header_text_light`, `header_text_dark`
- `text_secondary_light`, `text_secondary_dark`
- `card_bg_light`, `card_bg_dark`
- `theme_grey_light`, `theme_grey_dark`
- `keyboard_key_light`, `keyboard_key_dark`
- `keyboard_key_special_light`, `keyboard_key_special_dark`

Keep specialized colors that aren't part of the core semantic mapping (graph colors, mesh colors, etc.).

**Step 4: Build and test**

Run: `make -j && ./build/bin/helix-screen --test -vv`
Expected: UI should look identical

**Step 5: Commit**

```bash
git add src/ui/ui_theme.cpp ui_xml/globals.xml
git commit -m "feat(theme): implement semantic color mapping from palette"
```

---

## Phase 4: Theme Selector UI

Wire up the settings dropdown to select and load different themes.

### Task 4.1: Update SettingsManager for Theme Selection

**Files:**
- Modify: `include/settings_manager.h`
- Modify: `src/system/settings_manager.cpp`

**Step 1: Replace ThemePreset enum with string-based theme name**

In `include/settings_manager.h`, remove:
```cpp
enum class ThemePreset { NORD = 0 };
```

Update method signatures:
```cpp
    /** @brief Get current theme filename (without .json) */
    std::string get_theme_name() const;

    /** @brief Set theme by filename, marks restart pending */
    void set_theme_name(const std::string& name);

    /** @brief Get dropdown options string for discovered themes */
    std::string get_theme_options() const;

    /** @brief Get index of current theme in options list */
    int get_theme_index() const;

    /** @brief Set theme by dropdown index */
    void set_theme_by_index(int index);
```

Remove the old static methods:
```cpp
    // Remove: theme_preset_count(), get_theme_preset_name(), etc.
```

**Step 2: Implement in settings_manager.cpp**

Remove the hardcoded preset arrays. Add:

```cpp
#include "theme_loader.h"

std::string SettingsManager::get_theme_name() const {
    Config* config = Config::get_instance();
    return config ? config->get<std::string>("/display/theme", "nord") : "nord";
}

void SettingsManager::set_theme_name(const std::string& name) {
    spdlog::info("[SettingsManager] set_theme_name({})", name);

    Config* config = Config::get_instance();
    config->set<std::string>("/display/theme", name);
    config->save();

    restart_pending_ = true;
}

std::string SettingsManager::get_theme_options() const {
    auto themes = helix::discover_themes(helix::get_themes_directory());

    std::string options;
    for (size_t i = 0; i < themes.size(); ++i) {
        if (i > 0) options += "\n";
        options += themes[i].display_name;
    }
    return options;
}

int SettingsManager::get_theme_index() const {
    std::string current = get_theme_name();
    auto themes = helix::discover_themes(helix::get_themes_directory());

    for (size_t i = 0; i < themes.size(); ++i) {
        if (themes[i].filename == current) {
            return static_cast<int>(i);
        }
    }
    return 0;  // Default to first theme
}

void SettingsManager::set_theme_by_index(int index) {
    auto themes = helix::discover_themes(helix::get_themes_directory());

    if (index >= 0 && index < static_cast<int>(themes.size())) {
        set_theme_name(themes[index].filename);
    }
}
```

**Step 3: Commit**

```bash
git add include/settings_manager.h src/system/settings_manager.cpp
git commit -m "feat(settings): replace ThemePreset with dynamic theme selection"
```

### Task 4.2: Wire Up Theme Dropdown in UI

**Files:**
- Modify: `src/ui/ui_settings_display.cpp`

**Step 1: Update dropdown population**

Find the theme preset dropdown setup code and update to use new API:

```cpp
// Find the theme dropdown and populate with discovered themes
lv_obj_t* theme_dropdown = lv_obj_find_by_name(panel, "row_theme_preset");
if (theme_dropdown) {
    lv_obj_t* dropdown = lv_obj_find_by_name(theme_dropdown, "dropdown");
    if (dropdown) {
        std::string options = SettingsManager::instance().get_theme_options();
        lv_dropdown_set_options(dropdown, options.c_str());
        lv_dropdown_set_selected(dropdown, SettingsManager::instance().get_theme_index());
    }
}
```

**Step 2: Update callback handler**

```cpp
void on_theme_preset_changed(lv_event_t* e) {
    lv_obj_t* dropdown = lv_event_get_target(e);
    int index = lv_dropdown_get_selected(dropdown);

    SettingsManager::instance().set_theme_by_index(index);

    // Show restart required dialog
    show_theme_restart_dialog();
}
```

**Step 3: Build and test**

Run: `make -j && ./build/bin/helix-screen --test -vv`

1. Create a second theme file: `$HELIXDIR/config/themes/dracula.json`
2. Open Settings → Display → Theme
3. Verify dropdown shows both themes
4. Select different theme, verify restart dialog appears

**Step 4: Commit**

```bash
git add src/ui/ui_settings_display.cpp
git commit -m "feat(settings): wire up dynamic theme selector dropdown"
```

---

## Phase 5: Live Preview System

Extend helix_theme to support live color updates for preview.

### Task 5.1: Extend helix_theme_update_colors for Full Palette

**Files:**
- Modify: `include/helix_theme.h`
- Modify: `src/helix_theme.c`

**Step 1: Add extended update function declaration**

In `include/helix_theme.h`, add:

```c
/**
 * @brief Update all theme colors for live preview
 *
 * Updates theme styles in-place without requiring restart.
 * Call lv_obj_report_style_change(NULL) after to trigger refresh.
 *
 * @param is_dark Dark mode flag
 * @param colors Array of 16 hex color strings (palette order)
 * @param border_radius Corner radius in pixels
 */
void helix_theme_preview_colors(bool is_dark, const char* colors[16], int32_t border_radius);
```

**Step 2: Implement the function**

In `src/helix_theme.c`, add:

```c
void helix_theme_preview_colors(bool is_dark, const char* colors[16], int32_t border_radius) {
    if (!helix_theme_instance) {
        return;
    }

    // Parse palette colors
    // 0: bg_darkest, 1: bg_dark, 2: bg_dark_highlight, 3: border_muted
    // 4: text_light, 5: bg_light, 6: bg_lightest, 7: accent_highlight
    // 8-10: accents, 11-15: status

    lv_color_t screen_bg = is_dark ?
        lv_color_hex(strtoul(colors[0] + 1, NULL, 16)) :  // bg_darkest
        lv_color_hex(strtoul(colors[6] + 1, NULL, 16));   // bg_lightest

    lv_color_t card_bg = is_dark ?
        lv_color_hex(strtoul(colors[1] + 1, NULL, 16)) :  // bg_dark
        lv_color_hex(strtoul(colors[5] + 1, NULL, 16));   // bg_light

    lv_color_t theme_grey = is_dark ?
        lv_color_hex(strtoul(colors[3] + 1, NULL, 16)) :  // border_muted
        lv_color_hex(strtoul(colors[4] + 1, NULL, 16));   // text_light

    lv_color_t text_primary = is_dark ?
        lv_color_hex(strtoul(colors[6] + 1, NULL, 16)) :  // bg_lightest
        lv_color_hex(strtoul(colors[0] + 1, NULL, 16));   // bg_darkest

    // Update styles
    helix_theme_instance->is_dark_mode = is_dark;

    // Recompute input widget background color
    lv_color_t input_bg = compute_input_bg_color(card_bg, is_dark);
    lv_style_set_bg_color(&helix_theme_instance->input_bg_style, input_bg);

    // Update button style
    lv_style_set_bg_color(&helix_theme_instance->button_style, theme_grey);
    lv_style_set_text_color(&helix_theme_instance->button_style, text_primary);
    lv_style_set_radius(&helix_theme_instance->button_style, border_radius);
    lv_style_set_radius(&helix_theme_instance->pressed_style, border_radius);

    // Update default theme internal styles
    // (same private API access pattern as helix_theme_update_colors)
    typedef struct {
        lv_style_t scr;
        lv_style_t scrollbar;
        lv_style_t scrollbar_scrolled;
        lv_style_t card;
        lv_style_t btn;
    } theme_styles_partial_t;

    typedef struct {
        lv_theme_t base;
        int disp_size;
        int32_t disp_dpi;
        lv_color_t color_scr;
        lv_color_t color_text;
        lv_color_t color_card;
        lv_color_t color_grey;
        bool inited;
        theme_styles_partial_t styles;
    } default_theme_t;

    default_theme_t* def_theme = (default_theme_t*)helix_theme_instance->default_theme;

    def_theme->color_scr = screen_bg;
    def_theme->color_card = card_bg;
    def_theme->color_grey = theme_grey;
    def_theme->color_text = text_primary;

    lv_style_set_bg_color(&def_theme->styles.scr, screen_bg);
    lv_style_set_text_color(&def_theme->styles.scr, text_primary);
    lv_style_set_bg_color(&def_theme->styles.card, card_bg);
    lv_style_set_bg_color(&def_theme->styles.btn, theme_grey);
    lv_style_set_radius(&def_theme->styles.btn, border_radius);

    // Trigger style refresh
    lv_obj_report_style_change(NULL);
}
```

**Step 3: Commit**

```bash
git add include/helix_theme.h src/helix_theme.c
git commit -m "feat(theme): add helix_theme_preview_colors for live preview"
```

### Task 5.2: Add C++ Preview API

**Files:**
- Modify: `include/ui_theme.h`
- Modify: `src/ui/ui_theme.cpp`

**Step 1: Add preview function declaration**

In `include/ui_theme.h`:

```cpp
/**
 * @brief Preview theme colors without restart
 *
 * Applies theme colors for live preview. Call ui_theme_revert_preview()
 * to restore original colors, or restart to apply permanently.
 *
 * @param theme Theme data to preview
 */
void ui_theme_preview(const helix::ThemeData& theme);

/**
 * @brief Revert to active theme (cancel preview)
 */
void ui_theme_revert_preview();
```

**Step 2: Implement**

In `src/ui/ui_theme.cpp`:

```cpp
void ui_theme_preview(const helix::ThemeData& theme) {
    const char* colors[16];
    for (size_t i = 0; i < 16; ++i) {
        colors[i] = theme.colors.at(i).c_str();
    }

    helix_theme_preview_colors(use_dark_mode, colors, theme.properties.border_radius);
    ui_theme_refresh_widget_tree(lv_screen_active());

    spdlog::debug("[Theme] Previewing theme: {}", theme.name);
}

void ui_theme_revert_preview() {
    ui_theme_preview(active_theme);
    spdlog::debug("[Theme] Reverted to active theme: {}", active_theme.name);
}
```

**Step 3: Commit**

```bash
git add include/ui_theme.h src/ui/ui_theme.cpp
git commit -m "feat(theme): add ui_theme_preview API for live preview"
```

---

## Phase 6: Theme Editor UI

Make swatches clickable, add property sliders, and implement save/revert.

### Task 6.1: Create ThemeEditorOverlay Class

**Files:**
- Create: `include/ui_theme_editor_overlay.h`
- Create: `src/ui/ui_theme_editor_overlay.cpp`
- Modify: `Makefile`

This is a larger task. Create the overlay class that manages:
- Current editing state (dirty flag, modified theme)
- Color swatch click handlers
- Property slider handlers
- Save/Revert/Save As New buttons
- Dirty state confirmation dialogs

**Step 1: Create header**

```cpp
// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "overlay_base.h"
#include "theme_loader.h"

/**
 * @brief Theme editor overlay with live preview
 *
 * Allows editing theme colors and properties with immediate preview.
 * Tracks dirty state and prompts for save on exit.
 */
class ThemeEditorOverlay : public OverlayBase {
  public:
    ThemeEditorOverlay();
    ~ThemeEditorOverlay() override;

    void create(lv_obj_t* parent) override;
    void destroy() override;

    /** @brief Check if theme has unsaved changes */
    bool is_dirty() const { return dirty_; }

    /** @brief Load theme for editing */
    void load_theme(const std::string& filename);

  private:
    void setup_callbacks();
    void update_swatch_colors();
    void update_property_sliders();
    void mark_dirty();
    void clear_dirty();

    // Callbacks
    static void on_swatch_clicked(lv_event_t* e);
    static void on_slider_changed(lv_event_t* e);
    static void on_save_clicked(lv_event_t* e);
    static void on_save_as_clicked(lv_event_t* e);
    static void on_revert_clicked(lv_event_t* e);
    static void on_close_requested(lv_event_t* e);

    void show_color_picker(int palette_index);
    void show_save_as_dialog();
    void show_discard_confirmation(std::function<void()> on_discard);

    helix::ThemeData editing_theme_;
    helix::ThemeData original_theme_;
    bool dirty_ = false;
    int editing_color_index_ = -1;

    lv_obj_t* panel_ = nullptr;
    std::array<lv_obj_t*, 16> swatch_objects_{};
};
```

**Step 2: Implement basic structure**

Create `src/ui/ui_theme_editor_overlay.cpp` with the class implementation. This is substantial - implement incrementally:

1. Constructor/destructor
2. create() - find swatches by name, set up click handlers
3. load_theme() - copy theme data, update UI
4. Swatch click → show color picker overlay
5. Color picker callback → update theme, mark dirty, preview
6. Save/Revert/SaveAs button handlers
7. Close with dirty check

**Step 3: Update Makefile**

Add to SRC_FILES:
```makefile
src/ui/ui_theme_editor_overlay.cpp \
```

**Step 4: Build and test incrementally**

**Step 5: Commit**

```bash
git add include/ui_theme_editor_overlay.h src/ui/ui_theme_editor_overlay.cpp Makefile
git commit -m "feat(theme): implement ThemeEditorOverlay with live preview"
```

### Task 6.2: Add Property Sliders to XML

**Files:**
- Modify: `ui_xml/theme_settings_overlay.xml`

**Step 1: Add sliders section after color swatches**

```xml
<!-- Property Sliders -->
<setting_section_header title="STYLE PROPERTIES" icon="tune"/>

<setting_slider_row name="row_border_radius"
    label="Border Radius" icon="rounded-corner"
    description="Corner roundness (0 = sharp, 20 = very round)"
    min="0" max="20" callback="on_border_radius_changed"/>

<setting_slider_row name="row_border_width"
    label="Border Width" icon="border-style"
    description="Default border thickness"
    min="0" max="4" callback="on_border_width_changed"/>

<setting_slider_row name="row_border_opacity"
    label="Border Opacity" icon="opacity"
    description="Border transparency"
    min="0" max="255" callback="on_border_opacity_changed"/>

<setting_slider_row name="row_shadow_intensity"
    label="Shadow Intensity" icon="box-shadow"
    description="Drop shadow strength (0 = disabled)"
    min="0" max="50" callback="on_shadow_changed"/>
```

**Step 2: Add action buttons**

```xml
<!-- Action Buttons -->
<lv_obj name="action_buttons" width="100%" height="content"
    style_pad_all="#space_lg" flex_flow="row" style_pad_gap="#space_md"
    style_bg_opa="0" style_border_width="0">

    <button_secondary name="btn_revert" flex_grow="1">
        <text_body text="Revert"/>
    </button_secondary>

    <button_secondary name="btn_save_as" flex_grow="1">
        <text_body text="Save As New"/>
    </button_secondary>

    <button_primary name="btn_save" flex_grow="1">
        <text_body text="Save"/>
    </button_primary>
</lv_obj>
```

**Step 3: Commit**

```bash
git add ui_xml/theme_settings_overlay.xml
git commit -m "feat(theme): add property sliders and action buttons to editor"
```

### Task 6.3: Wire Up Slider and Button Callbacks

**Files:**
- Modify: `src/ui/ui_theme_editor_overlay.cpp`

Implement the callback handlers for sliders and buttons. Each slider change should:
1. Update `editing_theme_.properties.*`
2. Call `mark_dirty()`
3. Call `ui_theme_preview(editing_theme_)`

Save button should:
1. Call `helix::save_theme_to_file()`
2. Update config if filename changed
3. Call `clear_dirty()`
4. Show restart dialog

**Step 1-5: Implement callbacks**

**Step 6: Commit**

```bash
git add src/ui/ui_theme_editor_overlay.cpp
git commit -m "feat(theme): wire up slider and button callbacks"
```

### Task 6.4: Integrate Color Picker

**Files:**
- Modify: `src/ui/ui_theme_editor_overlay.cpp`

Wire up swatch clicks to open a color picker overlay. When color is selected:
1. Update `editing_theme_.colors.at(editing_color_index_)`
2. Update swatch background color
3. Mark dirty
4. Preview

**Step 1: Implement show_color_picker()**

Use existing color picker component or create simple one with hex input.

**Step 2: Commit**

```bash
git add src/ui/ui_theme_editor_overlay.cpp
git commit -m "feat(theme): integrate color picker for swatch editing"
```

### Task 6.5: Implement Dirty State Handling

**Files:**
- Modify: `src/ui/ui_theme_editor_overlay.cpp`

**Step 1: Implement discard confirmation**

When user tries to:
- Close overlay while dirty
- Select different theme while dirty
- Click Revert

Show confirmation dialog:
```
"You have unsaved changes. Discard?"
[Discard] [Cancel]
```

**Step 2: Add visual dirty indicator**

Update overlay title when dirty: "Theme Editor *"

**Step 3: Commit**

```bash
git add src/ui/ui_theme_editor_overlay.cpp
git commit -m "feat(theme): implement dirty state handling with confirmation"
```

### Task 6.6: Implement Save As New Dialog

**Files:**
- Modify: `src/ui/ui_theme_editor_overlay.cpp`

**Step 1: Create save dialog**

Show text input dialog for theme name. On save:
1. Sanitize name to filename
2. Check for collision, append number if needed
3. Save to new file
4. Update config to use new theme
5. Clear dirty
6. Show restart dialog

**Step 2: Commit**

```bash
git add src/ui/ui_theme_editor_overlay.cpp
git commit -m "feat(theme): implement Save As New with name dialog"
```

---

## Final Testing

### Task 7.1: End-to-End Testing

**Manual test checklist:**

1. **First run:** Verify `nord.json` is created in themes directory
2. **Theme loading:** Verify theme loads correctly from JSON
3. **Theme selector:** Verify dropdown shows all themes
4. **Theme switching:** Verify selecting different theme shows restart dialog
5. **Live preview:** Verify color changes preview immediately
6. **Property sliders:** Verify border_radius, etc. update preview
7. **Save:** Verify theme saves to JSON correctly
8. **Save As New:** Verify new theme file created with sanitized name
9. **Revert:** Verify reverts to original colors
10. **Dirty state:** Verify confirmation on close with unsaved changes
11. **Restart:** Verify new theme applies after restart

**Step 1: Run full test suite**

```bash
make test-run
```

**Step 2: Manual testing with app**

```bash
./build/bin/helix-screen --test -vv
```

**Step 3: Final commit**

```bash
git add -A
git commit -m "test(theme): complete end-to-end testing"
```

---

## Summary

| Phase | Tasks | Key Deliverables |
|-------|-------|------------------|
| 1 | 1.1-1.4 | ThemeLoader with JSON parse/save/discover |
| 2 | 2.1-2.2 | Integration with ui_theme_init |
| 3 | 3.1 | Semantic color mapping |
| 4 | 4.1-4.2 | Theme selector dropdown |
| 5 | 5.1-5.2 | Live preview system |
| 6 | 6.1-6.6 | Theme editor UI |
| 7 | 7.1 | End-to-end testing |

Total: ~20 tasks, each 2-15 minutes
