#!/bin/bash
# Copyright (C) 2025-2026 356C LLC
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Regenerate Noto Sans text fonts for LVGL with i18n support
#
# Includes character ranges for:
# - Basic ASCII (0x20-0x7F)
# - Latin-1 Supplement (0x00A0-0x00FF) - Western European: French, Spanish, German
# - Cyrillic (0x0400-0x04FF) - Russian, Ukrainian, etc.
# - Typography symbols (smart quotes, dashes, bullets)
# - Currency symbols (€, £, ¥)

set -e
cd "$(dirname "$0")/.."

# Add node_modules/.bin to PATH so lv_font_conv is available
export PATH="$PWD/node_modules/.bin:$PATH"

echo "Generating Noto Sans text fonts with i18n support..."

# Check lv_font_conv is available
if ! command -v lv_font_conv >/dev/null 2>&1; then
    echo "ERROR: lv_font_conv not found"
    echo "Run: npm install"
    exit 1
fi

# Font files
REGULAR="assets/fonts/NotoSans-Regular.ttf"
BOLD="assets/fonts/NotoSans-Bold.ttf"
LIGHT="assets/fonts/NotoSans-Light.ttf"

# Check fonts exist
for font in "$REGULAR" "$BOLD" "$LIGHT"; do
    if [ ! -f "$font" ]; then
        echo "ERROR: Font not found: $font"
        exit 1
    fi
done

# Character ranges for i18n support:
# - 0x20-0x7F: Basic ASCII
# - 0x00A0-0x00FF: Latin-1 Supplement (accented chars for Western European)
# - 0x0400-0x04FF: Cyrillic (Russian, Ukrainian, etc.)
# - Typography: smart quotes, dashes, bullets, ellipsis
# - Currency: Euro, Pound, Yen
RANGE="0x20-0x7F"           # Basic ASCII
RANGE+=",0x00A0-0x00FF"     # Latin-1 Supplement (ç, ñ, ü, ß, etc.)
RANGE+=",0x0400-0x04FF"     # Cyrillic (Russian alphabet)
RANGE+=",0x2013-0x2014"     # En-dash, em-dash
RANGE+=",0x2018-0x201D"     # Smart quotes
RANGE+=",0x2022"            # Bullet
RANGE+=",0x2026"            # Ellipsis
RANGE+=",0x2122"            # Trademark
RANGE+=",0x20AC"            # Euro

# Common options
OPTS="--bpp 4 --format lvgl --no-compress"

echo "  Font range: Basic ASCII + Latin-1 + Cyrillic + Typography"
echo ""

# Generate Regular weight fonts
echo "  Generating Noto Sans Regular..."
for size in 10 12 14 16 18 20 24 26 28; do
    echo "    → noto_sans_${size}.c"
    lv_font_conv --font "$REGULAR" --size $size $OPTS --range $RANGE -o "assets/fonts/noto_sans_${size}.c"
done

# Generate Bold weight fonts
echo "  Generating Noto Sans Bold..."
for size in 14 16 18 20 24 28; do
    echo "    → noto_sans_bold_${size}.c"
    lv_font_conv --font "$BOLD" --size $size $OPTS --range $RANGE -o "assets/fonts/noto_sans_bold_${size}.c"
done

# Generate Light weight fonts
echo "  Generating Noto Sans Light..."
for size in 10 12 14 16 18; do
    echo "    → noto_sans_light_${size}.c"
    lv_font_conv --font "$LIGHT" --size $size $OPTS --range $RANGE -o "assets/fonts/noto_sans_light_${size}.c"
done

echo ""
echo "Done! Generated fonts with i18n support."
echo ""
echo "Character coverage:"
echo "  - English, German, French, Spanish, Portuguese, Italian"
echo "  - Russian (Cyrillic)"
echo ""
echo "Rebuild required: make -j"
