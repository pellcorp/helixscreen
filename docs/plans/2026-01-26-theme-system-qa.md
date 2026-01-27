# Theme System QA

## Status: THEME PREVIEW COMPLETE - Individual Theme QA Remains

## Completed This Session

### Kanagawa Theme Fix
- **Swapped secondary ↔ tertiary colors** in both dark and light modes
- Dark: secondary=#957FB8 (oniViolet/purple), tertiary=#D27E99 (sakuraPink)
- Light: secondary=#624C83 (purple), tertiary=#B35B79 (pink)
- Result: Switch/slider tracks now use purple which pairs better with the light blue primary/handle color

### Checkbox Theming Fix
- Added `checkbox_text_style` to `theme_core.c`
- Checkboxes now properly use theme text color (was potentially hardcoded)
- Added checkbox to theme preview panel for visual verification

### Header Bar Dual Button Support
- `header_bar.xml` now supports TWO action buttons (action_button_2 left of action_button)
- Props: `action_button_2_text`, `action_button_2_bg_color`, `action_button_2_callback`, `hide_action_button_2`
- `overlay_panel.xml` passes through new props
- Both buttons use `text_button` widget for auto-centering, min-width=90px for consistency

### Theme Preview UI Improvements
- Edit + Apply buttons now in header bar (was: Apply in header, Edit buried at bottom)
- Edit uses secondary color, Apply uses primary color
- Button preview reorganized: Primary/Secondary row, Tertiary/Warning row, Danger full-width
- Fixed header background to use `app_bg` (was incorrectly using `card_bg` in light mode)
- Fixed back button to have transparent background during preview
- Added checkbox preview with "Enable" label

### Files Modified
- `config/themes/defaults/kanagawa.json` - Color swap
- `src/theme_core.c` - Checkbox text styling
- `src/ui/theme_manager.cpp` - Preview color updates, overlay finding
- `src/ui/ui_settings_display.cpp` - Header color fix, button updates
- `ui_xml/header_bar.xml` - Dual button support
- `ui_xml/overlay_panel.xml` - Pass through new props
- `ui_xml/theme_preview_overlay.xml` - New button layout

## Next: Global Theme System Improvements

### ⚠️ PRIORITY: Kill Legacy Theme System (BLOCKING)
**This is blocking other fixes!** The legacy `ThemePalette` has color names like `text_light`
that collide with our new naming (`text_light` = light mode's text color).

Files to audit/purge:
- `src/ui/theme_loader.cpp` - `ThemePalette` class, legacy parsing code
- `src/ui/theme_manager.cpp` - any legacy constant registration
- Any XML files using old color names: `bg_darkest`, `bg_dark`, `surface_elevated`,
  `surface_dim`, `text_light` (legacy), `bg_light`, `bg_lightest`, `accent_highlight`,
  `accent_primary`, `accent_secondary`, `accent_tertiary`, `status_error`, etc.

Tasks:
1. [ ] Remove `ThemePalette` class entirely
2. [ ] Remove legacy JSON parsing (`"colors"` object format)
3. [ ] Search codebase for legacy color name usage
4. [ ] Test all themes still load correctly

### Button Borders (Not Started)
- All 65+ XML files have `style_border_width="0"` hardcoded
- Buttons should respect theme `border_width`, `border_opacity`, and `border_radius`
- Need to expose these as XML constants (`#border_width`, `#border_opacity`)

### Shadow System (Not Started)
- `shadow_intensity` is dead code - stored in JSON, shown in editor, never applied
- "Raised" elements (modals, dialogs, dropdowns) should have configurable drop shadows
- Need to wire `shadow_intensity` to actual widget rendering in `theme_core.c`

### Theme Editor Bug (Not Started)
- User reported border color showing as "gold" in edit panel but actual value is purple-gray
- Need to investigate potential color display bug

### Theme Application Refactor (Not Started)
**Problem:** Theme styling logic is duplicated across multiple files:
- `theme_core.c` - initial widget styling on creation
- `ui_switch.cpp` - switch knob OFF state calculation
- `ui_settings_display.cpp` - manual preview color updates (50+ lines of `lv_obj_set_style_*`)

**Solution:** Centralize theme application:
1. Create `theme_manager_apply_to_widget(lv_obj_t* widget, ThemePalette* palette)` - knows how to style ANY widget type
2. Create `theme_manager_apply_to_tree(lv_obj_t* root, ThemePalette* palette)` - walks tree and applies to each widget
3. Replace manual preview styling with single call: `theme_manager_apply_to_tree(overlay, palette)`
4. Extract shared calculations (e.g., switch OFF knob color) into reusable functions

**Benefits:**
- Single source of truth for how each widget type gets themed
- Preview code becomes trivial
- Easier to add new themed widgets
- Less duplication, fewer bugs

### Remaining Theme QA - IN PROGRESS

#### Design Principles (Reference These!)

**1. Light Mode Background Pattern** - Theme-dependent, NOT one-size-fits-all:
- **Cards float (white/lightest)**: Ayu, Nord, OneDark, Material, Solarized, Tokyo Night
  - Modern clean look, cards "pop" above tinted background
- **Cards blend (darker than bg)**: Catppuccin, Gruvbox, Kanagawa
  - Warmer aesthetic, depth/layering feel, works with cream/sepia tones

**2. Primary/Secondary Pairing** - These appear together on switches/sliders:
- Secondary = track color, Primary = handle color
- Should complement each other visually, not clash
- Example fix: Kanagawa swapped secondary↔tertiary because pink clashed with blue

**3. text_subtle** - Must be a muted GRAY, not an accent color:
- Used for de-emphasized text (timestamps, hints, placeholders)
- Dark mode: lighter gray, Light mode: darker gray

#### Completed Fixes (This Session)
- ✅ Fixed `text_subtle` in 12 themes (was bright cyan/teal, now proper grays)
- ✅ Nord: swapped border↔text_subtle for better gray usage
- ✅ Switch OFF knob contrast - use card_alt (lighter) in dark mode, card_bg (darker) in light mode
- ✅ Disabled button text - Apply button now uses text_subtle when disabled
- ✅ Theme preview switch knob colors - fixed to use calculated OFF knob color, not primary
- ✅ Theme selection now updates all preview widgets (was only updating on dark mode toggle)
- ✅ Status icons now update colors on theme/mode change
- ✅ Dark Mode toggle switch styling fixed (was looking for wrong child element)
- ✅ Removed unused `get_contrasting_text_color()` and debug logging from ui_text.cpp

#### Session 2026-01-27 Changes

**Theme System Enhancements:**
- Added `button_radius` and `card_radius` XML constants from theme JSON `border_radius`
- Added `border_width` to theme system - buttons now get borders from theme
- Preview buttons/cards now use `#button_radius` and `#card_radius` instead of `#border_radius`

**ChatGPT Theme:**
- Deleted chatgpt-classic (was redundant)
- Updated ChatGPT with real current colors:
  - Dark: primary=#2E2E2E (neutral), secondary=#3C46FF (blue), tertiary=#0285FF, border=#505050
  - Light: primary=#FFFEFF (neutral), secondary=#3C46FF (blue), tertiary=#0285FF, border=#DAD9DA
  - border_radius=28 (pill buttons), shadow_intensity=30

#### Session 2026-01-27 Continued - Knob/Icon Color Fix & DRY Refactor

**Knob Color Fix (RESOLVED):**
- Changed from `brighter_color()` to `more_saturated_color()` - picks vivid accent over neutral
- Knobs use: `more_saturated_color(primary, tertiary)`
- Fixed ui_switch.cpp to set BOTH DEFAULT and CHECKED state knobs
- Fixed theme_manager_refresh_preview_elements for preview_dark_mode_toggle (was looking for wrong child)

**Icon Accent Fix:**
- Icons with `variant="accent"` now use `more_saturated_color(primary, secondary)`
- Fixed icons disappearing in light mode (near-white primary vs blue secondary)

**Theme Preview UI Tweaks:**
- Removed "Typography" header, moved "Heading" up (shorter card layout)
- Added tertiary "Open" button that opens sample modal with lorem ipsum and OK/Cancel

**DRY Refactor (theme_manager.h/cpp):**
- Added `theme_compute_saturation()` - compute HSV saturation (0-255)
- Added `theme_compute_more_saturated()` - return more vivid of two colors
- Added `theme_get_knob_color()` - returns `more_saturated(primary, tertiary)` from current theme
- Added `theme_get_accent_color()` - returns `more_saturated(primary, secondary)` from current theme
- Added `theme_apply_palette_to_widget()` - style ANY widget type from palette
- Added `theme_apply_palette_to_tree()` - walk tree and apply palette to all widgets
- Cleaned up ui_switch.cpp and ui_icon.cpp to use new helpers

**Files Modified:**
- include/theme_manager.h - new helper declarations
- include/ui_settings_display.h - on_preview_open_modal callback
- src/ui/theme_manager.cpp - saturation helpers, tree walker, preview element updates
- src/ui/ui_icon.cpp - use theme_get_accent_color()
- src/ui/ui_switch.cpp - use theme_get_knob_color()
- src/ui/ui_settings_display.cpp - modal callback, include ui_modal.h
- ui_xml/theme_preview_overlay.xml - layout changes, Open button, named icons

#### Session 2026-01-27 Evening - Dropdown & Cleanup

**Dropdown Selection Highlight FIXED:**
- LVGL dropdowns use `LV_PART_SELECTED` with state flags (CHECKED, PRESSED)
- Now setting styles for all state combinations in `theme_core.c`
- Dropdown accent color uses `more_saturated(primary, secondary)` to handle neutral themes like ChatGPT
- `dropdown_accent_color` stored in theme instance for use in apply callback
- `theme_apply_palette_to_screen_dropdowns()` handles already-open dropdowns during preview

**Code Quality Cleanup:**
- Replaced hardcoded colors and verbose `parse_hex_color` patterns across 7 files
- ChatGPT light mode `text_muted` corrected to #8F8F8F

**Divider Defaults:**
- Dividers now default to `border` color instead of `text_muted`
- Removed redundant opacity overrides from 11 XML files (were using 30-128, now use component default of 255)

**Modal Backdrop:**
- Backdrop opacity now configurable via `globals.xml` (commit 6c45028e)

#### QA Progress
- [x] ayu - DONE
- [x] catppuccin - DONE (both dark and light)
- [x] chatgpt - DONE (knobs/dropdowns use saturation-based accent color)
- [x] chatgpt-classic - DELETED (consolidated into chatgpt)
- [ ] dracula (dark only)
- [ ] everforest
- [ ] gruvbox
- [ ] material-design
- [ ] nord
- [ ] onedark
- [ ] rose-pine
- [ ] solarized
- [ ] tokyonight
- [ ] yami (dark only)

## How to Test

```bash
# View theme preview with Kanagawa
HELIX_THEME=kanagawa ./build/bin/helix-screen --test -v -p theme

# Test light mode toggle in the preview panel
```
