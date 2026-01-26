#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Module: forgex
# ForgeX firmware-specific: display config, screen.sh patching, stock UI handling
#
# Reads: SUDO
# Writes: (none)

# Source guard
[ -n "${_HELIX_FORGEX_SOURCED:-}" ] && return 0
_HELIX_FORGEX_SOURCED=1

# Configure ForgeX display settings for HelixScreen
# We use GUPPY mode because ForgeX handles backlight properly in this mode.
# STOCK mode expects ffstartup-arm to manage display/backlight which doesn't work for us.
# We disable GuppyScreen's init scripts so HelixScreen takes over the display.
configure_forgex_display() {
    var_file="/opt/config/mod_data/variables.cfg"
    guppy_init="/opt/config/mod/.root/S80guppyscreen"
    tslib_init="/opt/config/mod/.root/S35tslib"
    changed=false

    # Set display mode to GUPPY (required for backlight to work)
    if [ -f "$var_file" ]; then
        if grep -q "display[[:space:]]*=[[:space:]]*'STOCK'" "$var_file"; then
            log_info "Setting ForgeX display mode to GUPPY..."
            $SUDO sed -i "s/display[[:space:]]*=[[:space:]]*'STOCK'/display = 'GUPPY'/" "$var_file"
            changed=true
        elif grep -q "display[[:space:]]*=[[:space:]]*'HEADLESS'" "$var_file"; then
            log_info "Setting ForgeX display mode to GUPPY..."
            $SUDO sed -i "s/display[[:space:]]*=[[:space:]]*'HEADLESS'/display = 'GUPPY'/" "$var_file"
            changed=true
        fi
    fi

    # Disable GuppyScreen init script (remove execute permission)
    if [ -x "$guppy_init" ]; then
        log_info "Disabling GuppyScreen init script..."
        $SUDO chmod -x "$guppy_init"
        changed=true
    fi

    # Disable tslib init script (GuppyScreen's touch input layer)
    # HelixScreen uses its own input handling
    if [ -x "$tslib_init" ]; then
        log_info "Disabling tslib init script..."
        $SUDO chmod -x "$tslib_init"
        changed=true
    fi

    if [ "$changed" = true ]; then
        log_success "ForgeX configured for HelixScreen (GUPPY mode, GuppyScreen disabled)"
        return 0
    fi
    return 1
}

# Patch ForgeX screen.sh to skip non-100 backlight control when HelixScreen is active
# ForgeX's headless.cfg runs a delayed_gcode that dims the backlight 3 seconds after
# Klipper starts. This patch blocks dimming calls but allows the S99root 0→100 cycle.
#
# The smart patch:
# - Allows "backlight 100" (needed for S99root initialization cycle)
# - Blocks other values (10, 0, etc.) when helixscreen_active flag exists
patch_forgex_screen_sh() {
    screen_sh="/opt/config/mod/.shell/screen.sh"

    if [ ! -f "$screen_sh" ]; then
        log_info "ForgeX screen.sh not found, skipping patch"
        return 1
    fi

    # Check if already patched (look for the smart patch signature)
    if grep -q 'helixscreen_active.*!=.*100' "$screen_sh" 2>/dev/null; then
        log_info "ForgeX screen.sh already has smart patch"
        return 0
    fi

    # Remove old-style patch if present (blocks ALL backlight when flag exists)
    if grep -q "helixscreen_active" "$screen_sh" 2>/dev/null; then
        log_info "Removing old-style patch from screen.sh..."
        tmp_file="${screen_sh}.tmp"
        grep -v "helixscreen_active\|# Skip if HelixScreen" "$screen_sh" > "$tmp_file"
        $SUDO mv "$tmp_file" "$screen_sh"
        $SUDO chmod +x "$screen_sh"
    fi

    # Find the backlight) case and add our guard
    if ! grep -q "^[[:space:]]*backlight)" "$screen_sh"; then
        log_info "Could not find backlight case in screen.sh"
        return 1
    fi

    log_info "Patching ForgeX screen.sh with smart backlight control..."

    # Use awk to insert our check after "backlight)" line (BusyBox compatible)
    # Smart patch: only block non-100 values, allowing S99root's 0→100 init cycle
    tmp_file="${screen_sh}.tmp"
    awk '
    /^[[:space:]]*backlight\)/ {
        print
        print "        # Skip non-100 backlight changes when HelixScreen is controlling display"
        print "        # Allows S99root 0->100 init cycle but blocks Klipper eco dimming"
        print "        if [ -f /tmp/helixscreen_active ] && [ \"$2\" != \"100\" ]; then"
        print "            exit 0"
        print "        fi"
        next
    }
    { print }
    ' "$screen_sh" > "$tmp_file"

    if [ -s "$tmp_file" ] && grep -q 'helixscreen_active.*!=.*100' "$tmp_file" 2>/dev/null; then
        $SUDO mv "$tmp_file" "$screen_sh"
        $SUDO chmod +x "$screen_sh"
        log_success "ForgeX screen.sh patched with smart backlight control"
        return 0
    else
        rm -f "$tmp_file"
        log_warn "Failed to patch ForgeX screen.sh"
        return 1
    fi
}

# Remove HelixScreen patch from ForgeX screen.sh (for uninstall)
unpatch_forgex_screen_sh() {
    screen_sh="/opt/config/mod/.shell/screen.sh"

    if [ ! -f "$screen_sh" ]; then
        return 1
    fi

    # Check if patched
    if ! grep -q "helixscreen_active" "$screen_sh" 2>/dev/null; then
        log_info "ForgeX screen.sh not patched, nothing to remove"
        return 0
    fi

    log_info "Removing HelixScreen patch from ForgeX screen.sh..."

    # Use awk to remove only our specific block (BusyBox compatible)
    # Match and skip: comment line, if line with helixscreen_active, exit 0, fi
    tmp_file="${screen_sh}.tmp"
    awk '
    /# Skip if HelixScreen is controlling the display/ { skip=1; next }
    /if \[ -f \/tmp\/helixscreen_active \]; then/ { skip=1; next }
    skip && /^[[:space:]]*exit 0[[:space:]]*$/ { next }
    skip && /^[[:space:]]*fi[[:space:]]*$/ { skip=0; next }
    { print }
    ' "$screen_sh" > "$tmp_file"

    if [ -s "$tmp_file" ]; then
        $SUDO mv "$tmp_file" "$screen_sh"
        $SUDO chmod +x "$screen_sh"
    else
        rm -f "$tmp_file"
        log_warn "Failed to unpatch ForgeX screen.sh"
        return 1
    fi

    # Verify removal
    if grep -q "helixscreen_active" "$screen_sh" 2>/dev/null; then
        log_warn "Could not fully remove patch from screen.sh"
        return 1
    fi

    log_success "ForgeX screen.sh patch removed"
    return 0
}

# Disable stock FlashForge UI in auto_run.sh
# The stock firmware UI (ffstartup-arm/firmwareExe) is started by /opt/auto_run.sh
# which runs AFTER init scripts. We comment out the line to prevent it starting.
disable_stock_firmware_ui() {
    auto_run="/opt/auto_run.sh"
    if [ -f "$auto_run" ]; then
        # Check if ffstartup-arm line exists and is not already commented
        if grep -q "^/opt/PROGRAM/ffstartup-arm" "$auto_run"; then
            log_info "Disabling stock FlashForge UI in auto_run.sh..."
            # Comment out the ffstartup-arm line
            $SUDO sed -i 's|^/opt/PROGRAM/ffstartup-arm|# Disabled by HelixScreen: /opt/PROGRAM/ffstartup-arm|' "$auto_run"
            log_success "Stock FlashForge UI disabled"
            return 0
        fi
    fi
    return 1
}

# Re-enable stock FlashForge UI in auto_run.sh (for uninstall)
restore_stock_firmware_ui() {
    auto_run="/opt/auto_run.sh"
    if [ -f "$auto_run" ]; then
        # Check if our disabled line exists
        if grep -q "^# Disabled by HelixScreen: /opt/PROGRAM/ffstartup-arm" "$auto_run"; then
            log_info "Re-enabling stock FlashForge UI in auto_run.sh..."
            # Uncomment the ffstartup-arm line
            $SUDO sed -i 's|^# Disabled by HelixScreen: /opt/PROGRAM/ffstartup-arm|/opt/PROGRAM/ffstartup-arm|' "$auto_run"
            log_success "Stock FlashForge UI re-enabled"
            return 0
        fi
    fi
    return 1
}
