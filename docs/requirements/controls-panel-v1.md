# Controls Panel - UI Requirements v1

**Panel Name:** Controls Panel (Multi-Screen)
**Inspiration:** Bambu Lab X1C controls system with sub-screens
**Status:** Design Phase - Ready for Implementation
**Date:** 2025-10-12

---

## Overview

The Controls Panel provides manual printer control through a **launcher menu** that opens dedicated sub-screens for different control types. This follows Bambu Lab's pattern of organizing complex controls into focused, single-purpose screens.

**Architecture:**
- **Launcher Panel:** Card-based menu with 6 control categories
- **Sub-Screens:** Dedicated full-screen UI for each control type
- **Modal Components:** Reusable numeric keypad for temperature input
- **Navigation:** Back button on each sub-screen (like Print Detail overlay)

**Key Features:**
- Motion control with circular jog pad (XYZ movement + homing)
- Temperature management (nozzle + heatbed) with numeric keypad
- Extrusion controls (extrude/retract with safety checks)
- Real-time position and temperature feedback
- Preset temperature values for common materials
- Safety features (extrusion temp lock, confirmation dialogs)

---

## 1. Launcher Panel (Main Controls Menu)

### 1.1 Overall Layout
- **Layout Type:** Vertical flex container with card grid
- **Background:** Dark panel background (`#panel_bg` = `#1e1e1e`)
- **Padding:** 20px all sides
- **Grid:** 2 columns, 3 rows (6 cards total)
- **Gap:** 20px between cards

### 1.2 Card Specifications
- **Dimensions:**
  - Width: `(890px - 20px gap) / 2 = 435px` per card
  - Height: `180px` (consistent card height)
- **Layout:** Vertical flex (icon on top, label below)
- **Background:** `#card_bg` (`#2a2a2a`)
- **Border Radius:** 12px
- **Padding:** 24px all sides
- **Border:** None (clean card design)

### 1.3 Card Components
Each card contains:
1. **Icon** (FontAwesome 64px)
   - Size: 64×64px
   - Color: `#primary_color` (`#ff4444`) - red accent
   - Centered horizontally
   - Margin bottom: 16px

2. **Label** (montserrat_20)
   - Text: Card name (e.g., "Movement & Home")
   - Color: `#text_primary` (`#ffffff`)
   - Centered horizontally
   - Font: montserrat_20 (bold)

3. **Subtitle** (montserrat_16, optional)
   - Text: Brief description (e.g., "XYZ jog & homing")
   - Color: `#text_secondary` (`#b0b0b0`)
   - Centered horizontally
   - Margin top: 8px

### 1.4 Card List

| Card | Icon | Label | Subtitle | Action |
|------|------|-------|----------|--------|
| 1 | fa-arrows-up-down-left-right (`\uf0b2`) | Movement & Home | XYZ jog & homing | Open Motion sub-screen |
| 2 | fa-fire (`\uf06d`) | Nozzle Temp | Heat nozzle | Open Nozzle Temp sub-screen |
| 3 | fa-fire (`\uf06d`) | Heatbed Temp | Heat bed | Open Heatbed Temp sub-screen |
| 4 | fa-arrow-up-from-line (`\uf342`) | Extrude/Retract | Filament control | Open Extrusion sub-screen |
| 5 | fa-fan (`\uf863`) | Fan Control | Part cooling | Open Fan sub-screen (Phase 2) |
| 6 | fa-power-off (`\uf011`) | Motors Disable | Release steppers | Execute action + confirm |

**Note:** Card 6 (Motors Disable) may execute immediately with confirmation dialog rather than opening a sub-screen.

### 1.5 Interaction
- **Click:** Card press animation (scale 0.98, 100ms)
- **Release:** Navigate to appropriate sub-screen
- **Hover:** Subtle brightness increase (if mouse support available)

---

## 2. Motion Sub-Screen (XYZ Movement & Homing)

**✅ IMPLEMENTATION STATUS: COMPLETE (2025-10-13)**
- All jog pad buttons working with bold Unicode arrow icons (diagonal_arrows_40 font)
- Z-axis controls fully functional
- Distance selector with visual feedback
- Position display with reactive subject bindings
- Home buttons wired and functional
- Mock position simulation for testing

### 2.1 Screen Layout
- **Header Bar:** Back button (left) + "Motion: XYZ" title (center)
- **Content:** Two-column layout
  - **Left Column (65%):** Circular jog pad + distance selector + home buttons
  - **Right Column (35%):** Z-axis controls + real-time position display

### 2.2 Header Bar
- **Height:** 56px
- **Background:** `#1e1e1e` (darker than panel)
- **Layout:** Horizontal flex, space-between
- **Padding:** 12px horizontal

**Back Button:**
- **Type:** Icon + chevron (← or fa-chevron-left)
- **Size:** 40×40px clickable area
- **Icon Color:** `#text_primary`
- **Click:** Return to launcher panel
- **Pattern:** Same as Print Detail overlay back button

**Title:**
- **Text:** "Motion: XYZ"
- **Font:** montserrat_20 (bold)
- **Color:** `#text_primary`
- **Alignment:** Centered

### 2.3 Circular Jog Pad (XY Control)

**Design Note:** Implementation uses simplified 3×3 button grid instead of circular wedge design for easier implementation and maintenance. Functionally equivalent.

**Overall Design:**
- **Type:** ~~Circular button array with 8 directional wedges + center button~~ **3×3 button grid (IMPLEMENTED)**
- **Diameter:** 320px (visible circle) / **320×320px container (IMPLEMENTED)**
- **Background:** `#2a2a2a` (dark card bg)
- **Border:** 2px solid `#3a3a3a` (subtle ring)

**Construction (Simplified 3×3 Grid Implementation):**
1. **Base Container:**
   - `lv_obj` with 320×320px fixed size
   - ~~Circular clipping~~ **Rounded rectangle (12px radius)**
   - Background color: `#2a2a2a` / `#card_bg`
   - Border: 2px solid `#3a3a3a`

2. **8 Directional Buttons + Center Home:**
   - ~~Arc/wedge shaped buttons arranged radially~~ **3×3 grid of 80×80px square buttons**
   - ~~Each button: 45° wedge~~ **Standard lv_obj buttons with 8px border radius**
   - Directions: NW, N, NE (row 1), W, Home, E (row 2), SW, S, SE (row 3)
   - **Icons:** FontAwesome arrow icons (up, down, left, right, diagonals)
   - **Button Color:** `#3a3a3a` / `#jog_button_bg` (defined constant)
   - **Pressed Color:** `#ff4444` (red accent) - handled by LVGL
   - **Text Color:** `#ffffff` / `#text_primary` (white icons)
   - **Gap:** 12px between buttons

3. **Center Home Button:**
   - 80×80px button in center of grid
   - Icon: fa-home (`\uf015`) at 32px, color `#ff4444` / `#primary_color`
   - Background: `#card_bg_dark` (darker to differentiate)
   - **Action:** Home X and Y axes

**Interaction:**
- **Touch:** Press wedge to jog in that direction
- **Movement:** Single step per press (distance based on selector)
- **Visual Feedback:** Wedge turns red on press

### 2.4 Distance Selector
- **Position:** Below jog pad, centered
- **Layout:** Horizontal row of 4 radio buttons
- **Options:** `0.1mm`, `1mm`, `10mm`, `100mm`
- **Style:**
  - Button size: 70×36px
  - Selected: `#ff4444` background, white text
  - Unselected: `#3a3a3a` background, gray text
  - Border radius: 6px
  - Gap: 8px between buttons
- **Default:** `1mm` selected

### 2.5 Home Buttons Row
- **Position:** Below distance selector
- **Layout:** Horizontal row of 4 buttons
- **Buttons:** `Home All`, `Home X`, `Home Y`, `Home Z`
- **Style:**
  - Button size: 100×40px (slightly wider)
  - Background: `#3a3a3a`
  - Text: montserrat_16, white
  - Border radius: 6px
  - Gap: 12px between buttons
- **Action:** Send home command for respective axis

### 2.6 Z-Axis Controls (Right Column)
**Layout:** Vertical stack

1. **Z-Axis Button Group:**
   - 4 buttons stacked vertically
   - Button size: 100×48px each
   - Labels: `↑ 10`, `↑ 1`, `↓ 1`, `↓ 10`
   - Icons: fa-chevron-up / fa-chevron-down
   - Style: Same as jog pad buttons
   - Gap: 12px between buttons

2. **Home Z Button:**
   - Below Z buttons
   - Size: 100×40px
   - Label: "Home Z"
   - Style: Accent button (red background when pressed)

### 2.7 Real-Time Position Display
**Position:** Top of right column (above Z controls)

**Card Style:**
- Background: `#2a2a2a`
- Border radius: 8px
- Padding: 16px
- Width: 100% of column

**Content:**
```
Position:
X: 120.5 mm
Y: 105.2 mm
Z:  15.8 mm
```

- **Label:** "Position:" - montserrat_16, `#text_secondary`
- **Values:** montserrat_20 (monospace if available), `#text_primary`
- **Update:** Reactive binding to position subjects
- **Format:** Fixed 1 decimal place, right-aligned

---

## 3. Temperature Sub-Screens (Nozzle & Heatbed)

**Note:** Nozzle and Heatbed temperature screens share identical layout with different icons, labels, and presets.

### 3.1 Screen Layout
- **Header Bar:** Back button (left) + Title (center) + Confirm button (right)
- **Content:** Three sections
  - **Left (40%):** Visualization graphic
  - **Center (30%):** Current/Target temp + Preset buttons
  - **Right (30%):** Numeric keypad modal (slides in when needed)

### 3.2 Header Bar
- **Height:** 56px
- **Back Button:** Same as Motion screen
- **Title:** "Nozzle Temperature" or "Heatbed Temperature"
- **Confirm Button:**
  - Position: Top-right
  - Size: 120×40px
  - Label: "Confirm" (white text)
  - Background: `#4caf50` (green)
  - Border radius: 8px
  - **Action:** Apply temperature and close screen
  - **Enabled:** Only when temp value changed

### 3.3 Visualization Section (Left)

**Current Implementation (v1):**
- Fire icon (fa-fire, 64px) centered in 280×320px container
- Serves as visual reference for what's being controlled
- Background: `#card_bg` (#202020)

**Future Enhancement (v2+):**
- Real-time temperature graph showing current vs target over time
- Heating progress indicator (arc/gauge showing % to target)
- Visual feedback during heating (color changes, glow effects)
- Historical temperature data visualization

**Original Design (Deferred):**
- Nozzle/extruder illustration (similar to Bambu's)
- Heated bed illustration
- Images: `nozzle_viz.png`, `heatbed_viz.png` (to be created)
- Animation: Glowing effect when heating

### 3.4 Current/Target Display
**Position:** Top of center column

**Card Style:**
- Background: `#2a2a2a`
- Border radius: 8px
- Padding: 16px
- Width: 100%

**Content:**
```
Current / Target
210°C / 220°C
```

- **Format:** `{current}°C / {target}°C`
- **Font:** montserrat_28 (large, bold)
- **Color:** `#text_primary`
- **Alignment:** Centered
- **Update:** Real-time reactive binding (1s refresh)

### 3.5 Preset Buttons
**Position:** Below current/target display

**Button Grid:** 2 columns × 2 rows (4 preset buttons)
- Button size: 120×48px
- Gap: 12px between buttons
- Border radius: 6px

**Nozzle Presets:**
| Button | Label | Value |
|--------|-------|-------|
| 1 | Off | 0°C |
| 2 | PLA | 210°C |
| 3 | PETG | 240°C |
| 4 | ABS | 250°C |

**Heatbed Presets:**
| Button | Label | Value |
|--------|-------|-------|
| 1 | Off | 0°C |
| 2 | PLA | 60°C |
| 3 | PETG | 80°C |
| 4 | ABS | 100°C |

**Button Styles:**
- Background: `#3a3a3a`
- Text: montserrat_16, white
- Active: `#ff4444` background (when selected)
- **Action:** Set target temp to preset value

### 3.6 Custom Temperature Entry
**Position:** Below preset buttons

**Button:**
- Label: "Custom..."
- Size: 100% width × 48px height
- Style: Secondary button (`#3a3a3a` bg)
- **Action:** Open numeric keypad modal (slides in from right)

### 3.7 Status Message
**Position:** Bottom of center column

**Text:**
- Font: montserrat_14 (italic)
- Color: `#text_secondary`
- Alignment: Centered
- Max width: 100%

**Messages:**
- Nozzle: "Heating nozzle to target temperature..."
- Heatbed: "Bed will maintain temperature during printing to help material adhere better to the plate."

---

## 4. Numeric Keypad Modal Component (Reusable)

### 4.1 Modal Behavior
- **Type:** Slide-in overlay from right edge
- **Animation:** 300ms ease-out slide animation
- **Backdrop:** Semi-transparent black (`#000000`, opacity 128) - optional
- **Dismissal:** Click backdrop OR press ESC button
- **Z-Index:** Above all content (top layer)

### 4.2 Modal Dimensions
- **Width:** 380px (fixed)
- **Height:** 100% (full screen height)
- **Position:** Right edge of screen
- **Slide Start:** Off-screen right (+380px)
- **Slide End:** Docked to right edge (0px from right)

### 4.3 Modal Layout
**Background:** `#2a2a2a` (dark card)
**Padding:** 24px all sides

**Sections:**
1. **Input Display** (top)
2. **Keypad Grid** (center)
3. **Action Buttons** (bottom)

### 4.4 Input Display
**Position:** Top of modal

**Card Style:**
- Background: `#1e1e1e` (darker)
- Border radius: 8px
- Padding: 20px
- Height: 80px
- Margin bottom: 24px

**Content:**
```
       220 °C
```

- **Font:** montserrat_36 (extra large)
- **Color:** `#text_primary`
- **Alignment:** Right-aligned
- **Max Digits:** 3 digits (max 999°C)
- **Cursor:** Blinking cursor (optional)

### 4.5 Keypad Grid
**Layout:** 4 columns × 4 rows = 16 buttons

**Button Dimensions:**
- Size: 72×64px each
- Gap: 12px between buttons
- Border radius: 8px

**Button Layout:**
```
[  7  ] [  8  ] [  9  ] [ ← ]
[  4  ] [  5  ] [  6  ] [ESC]
[  1  ] [  2  ] [  3  ] [    ]
[  0  ] [  .  ] [  -  ] [ OK ]
```

**Button Styles:**
- **Number buttons (0-9):**
  - Background: `#3a3a3a`
  - Text: montserrat_24, white
  - Press: `#4a4a4a` (lighter)

- **Backspace (←):**
  - Background: `#3a3a3a`
  - Icon: fa-backspace or text "←"
  - Color: `#ff9800` (orange accent)

- **ESC:**
  - Background: `#555555` (mid-gray)
  - Text: "ESC" montserrat_16
  - Color: white
  - **Action:** Cancel and close modal

- **Decimal (.):**
  - Background: `#3a3a3a`
  - Text: "." montserrat_24
  - **Disabled:** For integer-only mode (temp doesn't use decimals)

- **Negative (-):**
  - Background: `#3a3a3a`
  - Text: "−" montserrat_24
  - **Disabled:** Temperature cannot be negative

- **OK:**
  - Background: `#4caf50` (green)
  - Text: "OK" montserrat_20 (bold)
  - Color: white
  - Height: 64px (spans 1 row)
  - **Action:** Confirm input and close modal

### 4.6 Keypad Behavior
1. **Number Press:** Append digit to current value (max 3 digits)
2. **Backspace:** Delete last digit
3. **ESC:** Cancel input, close modal, revert to previous value
4. **OK:** Confirm input, close modal, apply new temperature target
5. **Invalid Input:** Visual feedback (red border flash) for out-of-range values

**Validation:**
- Min temperature: 0°C
- Max temperature: 350°C (nozzle), 150°C (bed)
- Auto-clamp on confirmation

### 4.7 API Properties (for Reusability)
The numeric keypad should accept these properties:

```xml
<component name="numeric_keypad_modal">
  <api>
    <prop type="int" name="initial_value" default="0"/>
    <prop type="int" name="min_value" default="0"/>
    <prop type="int" name="max_value" default="999"/>
    <prop type="string" name="unit_label" default=""/>
    <prop type="bool" name="allow_decimal" default="false"/>
    <prop type="bool" name="allow_negative" default="false"/>
  </api>
</component>
```

**Usage Examples:**
```cpp
// Nozzle temperature
const char* attrs[] = {
    "initial_value", "210",
    "min_value", "0",
    "max_value", "350",
    "unit_label", "°C",
    NULL
};

// Bed temperature
const char* attrs[] = {
    "initial_value", "60",
    "min_value", "0",
    "max_value", "150",
    "unit_label", "°C",
    NULL
};

// Extrusion amount (with decimal)
const char* attrs[] = {
    "initial_value", "10",
    "min_value", "0",
    "max_value", "100",
    "unit_label", "mm",
    "allow_decimal", "true",
    NULL
};
```

---

## 5. Extrusion Sub-Screen

### 5.1 Screen Layout
- **Header Bar:** Back button (left) + "Extrude / Retract" title (center)
- **Content:** Two-column layout
  - **Left (50%):** Extruder visualization + status
  - **Right (50%):** Controls (amount selector + extrude/retract buttons)

### 5.2 Header Bar
- Same as Motion screen (back button + title)

### 5.3 Extruder Visualization (Left Column)
**Graphic:**
- Size: 280×320px
- Image: `extruder_viz.png` (to be created)
- Shows filament path and extruder motor
- Animation: Rotation when extruding (optional)

**Temperature Status Card:**
- Position: Below visualization
- Background: `#2a2a2a`
- Border radius: 8px
- Padding: 16px

**Content:**
```
Nozzle Temperature
210°C / 220°C ✓
```

- **Status Indicator:**
  - ✓ Green checkmark: Ready (temp within 5°C of target)
  - ⚠ Yellow warning: Heating (temp below target)
  - ✗ Red X: Too cold to extrude (<170°C)

### 5.4 Amount Selector (Right Column)
**Position:** Top of right column

**Label:**
- Text: "Extrusion Amount:"
- Font: montserrat_16
- Color: `#text_secondary`
- Margin bottom: 12px

**Radio Button Group:**
- Layout: Horizontal row (4 options)
- Options: `5mm`, `10mm`, `25mm`, `50mm`
- Button size: 80×40px
- Style:
  - Selected: `#ff4444` background, white text
  - Unselected: `#3a3a3a` background, gray text
  - Border radius: 6px
  - Gap: 8px
- **Default:** `10mm` selected

### 5.5 Extrude Button
**Position:** Below amount selector

**Button Style:**
- Size: 100% width × 72px height
- Background: `#4caf50` (green)
- Border radius: 8px
- Icon + Text layout:
  - Icon: fa-arrow-up (`\uf062`) at 32px, left aligned
  - Text: "Extrude" montserrat_20 (bold), centered
- **Disabled State:**
  - Background: `#3a3a3a` (gray)
  - Text color: `#666666`
  - Disabled when nozzle < 170°C

**Action:**
- Send extrude command with selected amount
- Visual feedback: Button press animation + loading state

### 5.6 Retract Button
**Position:** Below extrude button (12px gap)

**Button Style:**
- Size: 100% width × 72px height
- Background: `#ff9800` (orange)
- Border radius: 8px
- Icon + Text layout:
  - Icon: fa-arrow-down (`\uf063`) at 32px, left aligned
  - Text: "Retract" montserrat_20 (bold), centered
- **Disabled State:** Same as Extrude button

**Action:**
- Send retract command with selected amount (negative value)
- Visual feedback: Button press animation + loading state

### 5.7 Safety Warning
**Position:** Bottom of right column

**Warning Card:**
- Background: `#ff4444` with low opacity (rgba(255, 68, 68, 0.2))
- Border: 2px solid `#ff4444`
- Border radius: 8px
- Padding: 16px
- **Visibility:** Only shown when nozzle < 170°C

**Content:**
```
⚠ Nozzle must be >170°C to extrude

Current: 25°C
Target: 0°C
```

- Icon: ⚠ warning symbol
- Font: montserrat_14
- Color: `#ff4444`
- Includes current and target temps

---

## 6. Navigation Pattern

### 6.1 Back Button (Reusable Pattern)
**Consistent across all sub-screens:**

**Position:** Top-left of header bar
**Size:** 40×40px clickable area
**Icon:**
- Option 1: Text chevron "<" (montserrat_24)
- Option 2: fa-chevron-left icon (`\uf053`, 24px)

**Style:**
- Background: Transparent
- Icon color: `#text_primary` (`#ffffff`)
- Press: 50% opacity reduction
- No border, no padding visual

**Behavior:**
- Click: Return to previous screen
- Animation: Slide-out transition (300ms)
- From sub-screen → Launcher panel
- From launcher → Main navigation (Controls panel disappears)

**Implementation:** Same pattern as Print Detail overlay back button

### 6.2 Screen Transitions
**Entry Animation (Launcher → Sub-Screen):**
- Sub-screen slides in from right edge
- Duration: 300ms
- Easing: ease-out
- Launcher fades to black backdrop (optional)

**Exit Animation (Sub-Screen → Launcher):**
- Sub-screen slides out to right edge
- Duration: 300ms
- Easing: ease-in
- Launcher fades back in

---

## 7. Reactive Data Bindings (Subject-Observer)

### 7.1 Position Subjects (Motion Screen)
```cpp
lv_subject_t position_x_subject;    // Current X position (float → string)
lv_subject_t position_y_subject;    // Current Y position (float → string)
lv_subject_t position_z_subject;    // Current Z position (float → string)
```

**Format:** "120.5 mm" (1 decimal place)
**Update Rate:** On position change (from printer feedback)

### 7.2 Temperature Subjects
```cpp
// Nozzle
lv_subject_t nozzle_current_temp_subject;  // Current nozzle temp (int)
lv_subject_t nozzle_target_temp_subject;   // Target nozzle temp (int)

// Heatbed
lv_subject_t bed_current_temp_subject;     // Current bed temp (int)
lv_subject_t bed_target_temp_subject;      // Target bed temp (int)
```

**Format:** "210 / 220°C" (current / target)
**Update Rate:** 1-2 seconds (from printer feedback)

### 7.3 UI State Subjects
```cpp
lv_subject_t active_subscreen_subject;     // Integer: 0=launcher, 1=motion, 2=nozzle, etc.
lv_subject_t jog_distance_subject;         // Integer: 0=0.1mm, 1=1mm, 2=10mm, 3=100mm
lv_subject_t extrude_amount_subject;       // Integer: 0=5mm, 1=10mm, 2=25mm, 3=50mm
lv_subject_t keypad_visible_subject;       // Boolean: Keypad modal shown/hidden
```

### 7.4 Safety State Subjects
```cpp
lv_subject_t extrusion_allowed_subject;    // Boolean: Nozzle hot enough for extrusion
lv_subject_t homing_in_progress_subject;   // Boolean: Homing operation active
```

---

## 8. Color Palette & Typography

### 8.1 Colors (from globals.xml)
```xml
<color name="panel_bg" value="0x1e1e1e"/>         <!-- Panel background -->
<color name="card_bg" value="0x2a2a2a"/>          <!-- Card background -->
<color name="primary_color" value="0xff4444"/>    <!-- Red accent -->
<color name="text_primary" value="0xffffff"/>     <!-- White text -->
<color name="text_secondary" value="0xb0b0b0"/>   <!-- Gray text -->
<color name="success_color" value="0x4caf50"/>    <!-- Green (confirm, extrude) -->
<color name="warning_color" value="0xff9800"/>    <!-- Orange (retract, warnings) -->
<color name="error_color" value="0xf44336"/>      <!-- Red (errors) -->
```

### 8.2 Typography
```xml
<!-- Headers -->
montserrat_28 - Large display text (current/target temps)
montserrat_20 - Card titles, button labels
montserrat_16 - Body text, subtitles

<!-- Keypad -->
montserrat_36 - Keypad input display
montserrat_24 - Keypad number buttons

<!-- Icons -->
fa_icons_64 - Launcher card icons
fa_icons_32 - Sub-screen icons, jog pad
fa_icons_16 - Small status icons
```

### 8.3 Icon List (FontAwesome 6)
New icons needed for Controls Panel:

```cpp
// Motion & Movement
#define ICON_ARROWS_ALL     "\uf0b2"  // fa-arrows-up-down-left-right
#define ICON_HOME           "\uf015"  // fa-home
#define ICON_CHEVRON_UP     "\uf077"  // fa-chevron-up
#define ICON_CHEVRON_DOWN   "\uf078"  // fa-chevron-down
#define ICON_CHEVRON_LEFT   "\uf053"  // fa-chevron-left

// Temperature
#define ICON_FIRE           "\uf06d"  // fa-fire
#define ICON_THERMOMETER    "\uf2c8"  // fa-thermometer-half

// Extrusion
#define ICON_ARROW_UP_LINE  "\uf342"  // fa-arrow-up-from-line
#define ICON_ARROW_UP       "\uf062"  // fa-arrow-up
#define ICON_ARROW_DOWN     "\uf063"  // fa-arrow-down

// Other Controls
#define ICON_FAN            "\uf863"  // fa-fan
#define ICON_POWER_OFF      "\uf011"  // fa-power-off

// Keypad
#define ICON_BACKSPACE      "\uf55a"  // fa-delete-left
```

**Action:** Run `python3 scripts/generate-icon-consts.py` after adding to `ui_fonts.h`

---

## 9. Component File Structure

### 9.1 XML Files
```
ui_xml/
├── controls_panel.xml                    # Launcher panel (6 cards)
├── controls_motion_panel.xml             # Motion sub-screen (jog pad + Z)
├── controls_nozzle_temp_panel.xml        # Nozzle temperature sub-screen
├── controls_bed_temp_panel.xml           # Heatbed temperature sub-screen
├── controls_extrusion_panel.xml          # Extrusion sub-screen
├── numeric_keypad_modal.xml              # Reusable keypad component
└── (future) controls_fan_panel.xml       # Fan control sub-screen
```

### 9.2 C++ Files
```
src/
├── ui_panel_controls.cpp                 # Launcher + navigation logic
├── ui_panel_controls_motion.cpp          # Motion screen logic
├── ui_panel_controls_temp.cpp            # Temperature screens logic (shared)
├── ui_panel_controls_extrusion.cpp       # Extrusion screen logic
└── ui_components_keypad.cpp              # Keypad modal component logic

include/
├── ui_panel_controls.h
├── ui_panel_controls_motion.h
├── ui_panel_controls_temp.h
├── ui_panel_controls_extrusion.h
└── ui_components_keypad.h
```

### 9.3 Asset Files (to be created)
```
assets/images/
├── nozzle_viz.png                        # Nozzle/extruder illustration
├── heatbed_viz.png                       # Heated bed illustration
└── extruder_viz.png                      # Extruder motor illustration
```

---

## 10. Implementation Phases

### Phase 1: Foundation (PRIORITY)
- [ ] Add new icons to `ui_fonts.h` and regenerate constants
- [ ] Create `controls_panel.xml` with 6 launcher cards
- [ ] Create `ui_panel_controls.cpp` for launcher logic
- [ ] Wire launcher cards to placeholder click handlers
- [ ] Test navigation from main navbar to launcher

### Phase 2: Reusable Components (PRIORITY)
- [ ] Create `numeric_keypad_modal.xml` with slide-in animation
- [ ] Create `ui_components_keypad.cpp` with API properties
- [ ] Implement keypad input logic (number entry, backspace, validation)
- [ ] Test keypad modal opening/closing with ESC and OK

### Phase 3: Motion Sub-Screen (HIGH)
- [ ] Create `controls_motion_panel.xml` with header bar
- [ ] Implement circular jog pad with LVGL primitives (arc buttons?)
- [ ] Add Z-axis control buttons
- [ ] Implement distance selector (radio buttons)
- [ ] Add home buttons (All, X, Y, Z)
- [ ] Add real-time position display with reactive subjects
- [ ] Wire back button to return to launcher

### Phase 4: Temperature Sub-Screens (HIGH)
- [ ] Create `controls_nozzle_temp_panel.xml`
- [ ] Create `controls_bed_temp_panel.xml`
- [ ] Create or find visualization assets (nozzle_viz.png, heatbed_viz.png)
- [ ] Implement preset buttons
- [ ] Wire "Custom" button to open keypad modal
- [ ] Add current/target temp display with reactive subjects
- [ ] Implement Confirm button behavior
- [ ] Wire back button

### Phase 5: Extrusion Sub-Screen (HIGH)
- [ ] Create `controls_extrusion_panel.xml`
- [ ] Create or find extruder visualization asset
- [ ] Implement amount selector (radio buttons)
- [ ] Add Extrude/Retract buttons with enable/disable logic
- [ ] Add temperature safety warning card
- [ ] Wire back button

### Phase 6: Polish & Testing (MEDIUM)
- [ ] Add screen transition animations (slide in/out)
- [ ] Add button press animations and feedback
- [ ] Test all navigation paths
- [ ] Test with mock printer data (position, temps)
- [ ] Create UI requirements changelog document
- [ ] Screenshot testing for each sub-screen

### Phase 7: Advanced Features (LOW - Future)
- [ ] Fan control sub-screen
- [ ] Feed rate override sub-screen
- [ ] Visualization animations (glowing when heating, rotation when extruding)
- [ ] Confirmation dialog for Motors Disable
- [ ] Help text / tooltips

---

## 11. Open Questions & Decisions Needed

### 11.1 Circular Jog Pad Implementation
**Question:** How to build the circular jog pad with LVGL primitives?

**Options:**
1. **Custom draw callback:** Draw wedges in `LV_EVENT_DRAW_MAIN`
2. **Arc widgets:** Use 8 `lv_arc` widgets with narrow angles
3. **Image-based:** Pre-render jog pad as PNG with button overlays
4. **Button grid:** 3×3 button grid styled to look circular

**Recommendation:** Start with **Option 4 (button grid)** for v1, then enhance to Option 1 or 2 later.

**Button Grid Layout (3×3 = 9 buttons):**
```
[ ↖ ] [ ↑ ] [ ↗ ]    (-X,+Y)  (+Y)  (+X,+Y)
[ ← ] [🏠] [ → ]    (-X)   (Home)  (+X)
[ ↙ ] [ ↓ ] [ ↘ ]    (-X,-Y)  (-Y)  (+X,-Y)
```

- Button size: 90×90px each
- Corner buttons: Diagonal movement (X+Y combined)
- Center button: Home XY
- Style with circular clipping mask if possible

### 11.2 Temperature Visualization Assets
**Question:** Should we create custom illustrations or use generic icons?

**Options:**
1. Create custom SVG illustrations (Bambu-style)
2. Use enlarged FontAwesome icons (fire, thermometer)
3. Use simple geometric shapes
4. Skip visualization for v1, add later

**Recommendation:** **Option 2 for v1** (enlarged FA icons), add custom illustrations in Phase 6.

### 11.3 Screen Transition Animations
**Question:** Can LVGL 9 handle slide-in animations natively?

**Investigation Needed:**
- Check `lv_obj_set_style_translate_x()` animation support
- Test with dummy panels in prototype
- Fallback: Instant show/hide if animations not feasible

### 11.4 Real-Time Position Data
**Question:** How frequently should position update?

**Recommendation:**
- Update on position change (event-driven from printer)
- Fallback: Poll every 500ms-1s if no event support
- Don't update during jogging (too frequent)

---

## 12. Success Criteria

### 12.1 Functionality
- [ ] All launcher cards navigate to correct sub-screens
- [ ] Back button returns to launcher from all sub-screens
- [ ] Numeric keypad opens/closes smoothly
- [ ] Temperature presets apply correctly
- [ ] Motion controls send correct jog commands
- [ ] Extrusion safety checks work (temp < 170°C blocks extrusion)
- [ ] Real-time position display updates from printer data

### 12.2 Visual Quality
- [ ] Matches Bambu Lab design aesthetic (clean, modern, dark theme)
- [ ] Consistent spacing and alignment across all screens
- [ ] Icons render clearly at all sizes
- [ ] Text is legible and properly sized
- [ ] Cards and buttons have appropriate visual feedback

### 12.3 Usability
- [ ] Navigation is intuitive (easy to return to launcher)
- [ ] Keypad is easy to use (large buttons, clear feedback)
- [ ] Safety warnings are clear and prominent
- [ ] Temperature presets are convenient (one-tap access)
- [ ] Jog pad is responsive and accurate

### 12.4 Performance
- [ ] Screen transitions are smooth (no lag)
- [ ] Keypad appears instantly when tapped
- [ ] No frame drops during animations
- [ ] Real-time data updates without UI stutter

---

## 13. Future Enhancements

### 13.1 Phase 7 Features
- Bed mesh visualization (heatmap)
- PID tuning controls
- Z-offset calibration
- Webcam view integration
- Filament sensor status

### 13.2 Advanced Motion
- Continuous jogging (press and hold)
- Acceleration profiles
- Macro buttons (center bed, park head, etc.)

### 13.3 Temperature Profiles
- Save custom temperature presets
- Material profiles (PLA, PETG, etc. with multiple temps)
- Preheat sequences

### 13.4 Accessibility
- Larger button modes (for gloves)
- Voice feedback
- High contrast theme

---

**Document Version:** 1.0
**Last Updated:** 2025-10-12
**Status:** Ready for Implementation
**Next Step:** Implement Phase 1 (Launcher Panel)
