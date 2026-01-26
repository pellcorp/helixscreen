# AD5M Boot Process Notes

## ForgeX Firmware Configuration

### Display Mode (variables.cfg)
- **GUPPY mode** is what we want for HelixScreen (NOT STOCK!)
- Location: `/opt/config/mod_data/variables.cfg`
- Setting: `display = 'GUPPY'`
- STOCK mode expects ffstartup-arm to handle display/backlight - doesn't work for us
- GUPPY mode handles backlight properly via ForgeX scripts
- The `reset_screen` delayed_gcode just sets backlight level, doesn't draw to FB

### Disable GuppyScreen
- Remove execute permission from init scripts:
  - `/opt/config/mod/.root/S80guppyscreen`
  - `/opt/config/mod/.root/S35tslib`
- Command: `chmod -x /opt/config/mod/.root/S80guppyscreen /opt/config/mod/.root/S35tslib`

### Disable Stock FlashForge UI
- Comment out in `/opt/auto_run.sh`:
  - `/opt/PROGRAM/ffstartup-arm -f /opt/PROGRAM/ffstartup.cfg &`

## Backlight Control

### The Bug We Fixed (2024-01-24)
The `enable_backlight()` function had a path bug:
- `FORGEX_BACKLIGHT="/root/printer_data/py/backlight.py"` - path INSIDE chroot
- `FORGEX_CHROOT="/data/.mod/.forge-x"` - chroot location
- **BUG**: Checked `[ -x "$FORGEX_BACKLIGHT" ]` which tested `/root/printer_data/py/backlight.py` on HOST
- **FIX**: Check `[ -x "${FORGEX_CHROOT}${FORGEX_BACKLIGHT}" ]` for full path

### How Backlight Works on AD5M
- Uses custom ioctl to `/dev/disp` (not sysfs)
- ForgeX provides `backlight.py` script inside chroot
- Must run via chroot: `/usr/sbin/chroot /data/.mod/.forge-x /root/printer_data/py/backlight.py 100`
- In STOCK mode, ForgeX turns OFF backlight - we must turn it ON

### Boot Sequence (STOCK mode)
1. ForgeX S99root runs, shows "Booting stock firmware..."
2. ForgeX turns OFF backlight (in STOCK mode)
3. Our init script in `/etc/init.d/S99helixscreen` starts
4. We call `enable_backlight()` to turn ON backlight
5. We start splash, then helix-screen

## Debugging

### Enable Debug Logging
Add to init script (after LOGFILE line):
```bash
export HELIX_DEBUG=1
```

### Key Log File
- `/tmp/helixscreen.log`

### Manual Backlight Test
```bash
/usr/sbin/chroot /data/.mod/.forge-x /root/printer_data/py/backlight.py 100
```

### Framebuffer Info
- Device: `/dev/fb0`
- Resolution: 800x480
- Bits per pixel: 32
- Stride: 3200 bytes

## Init Script Locations

**IMPORTANT**: There are TWO copies of the init script:
1. `/etc/init.d/S90helixscreen` - **THIS IS WHAT RUNS AT BOOT**
2. `/opt/helixscreen/config/helixscreen.init` - source copy

When deploying fixes, must update `/etc/init.d/S90helixscreen`!

## SSH/SCP Notes

- AD5M BusyBox doesn't have sftp-server
- Must use legacy SCP protocol: `scp -O` flag
- Example: `scp -O localfile root@192.168.1.67:/path/`
- AD5M IP: `192.168.1.67` (use `AD5M_HOST=192.168.1.67` with make commands)
- mDNS (ad5m.local) may not resolve - use IP directly

## Boot Sequence (Detailed)

```
S00init  - Writes ForgeX splash to fb0, mounts /dev into chroot
S55boot  - Calls boot.sh, prints "Booting stock firmware..." via `logged --send-to-screen`
S90helixscreen - OUR SCRIPT: enables backlight, starts HelixScreen
S98camera, S98zssh
S99root  - Starts chroot services (Moonraker, etc.)
```

## Current Bug (2024-01-24): Backlight Not Turning On

**Symptom**: After "Booting stock firmware..." message, backlight turns off and stays off.

**What we've confirmed**:
- Our init script (S90helixscreen) DOES run
- `enable_backlight()` IS called
- The chroot backlight command DOES execute (returns 0)
- But backlight stays OFF

**Theory**: The backlight.py ioctl command is ACCEPTED (returns 0) but IGNORED
during cold boot. Even 60 seconds of repeated attempts doesn't help.
Manual command works AFTER boot is fully complete.

**NOT a timing issue** - tried repeated attempts over 60 seconds, none worked.

**Possible causes**:
1. Display driver not fully initialized during init scripts
2. ForgeX chroot environment missing something during boot
3. Hardware state requires stock firmware to initialize first
4. ioctl is queued but not executed until something else happens

**Key discovery**: Backlight IS on when "Booting stock firmware..." is shown,
then turns OFF. Stock firmware (ffstartup-arm) normally keeps display alive.
We disabled it, so nothing maintains the backlight.

**WORKING SOLUTION**:
- Use GUPPY mode (`display = 'GUPPY'` in variables.cfg)
- Disable GuppyScreen init scripts (chmod -x S80guppyscreen and S35tslib)
- Disable stock firmware in auto_run.sh
- ForgeX handles backlight in GUPPY mode
- helixscreen_active flag prevents S99root from drawing to screen

**Files involved**:
- `/etc/init.d/S90helixscreen` - our init script (THE ONE THAT RUNS)
- `/opt/helixscreen/config/helixscreen.init` - source copy (NOT used at boot!)
- `/opt/config/mod/.shell/S55boot` - prints "Booting stock firmware..."
- `/opt/config/mod/.bin/logged` - binary that draws text to screen

**Debug log location**: `/tmp/helix_boot.log`

## GuppyScreen Approach (2026-01-24)

GuppyScreen (also LVGL-based) works perfectly with ForgeX. Key insight: it uses
**standard Linux framebuffer ioctls** to unblank the display, not the custom
backlight.py script.

### fbdev_unblank() from GuppyScreen

Located in `lv_drivers/display/fbdev.c` (via patch):

```c
void fbdev_unblank(void) {
    // 1. Unblank via standard ioctl
    ioctl(fbfd, FBIOBLANK, FB_BLANK_UNBLANK);

    // 2. Get screen info and reset pan position
    struct fb_var_screeninfo var_info;
    ioctl(fbfd, FBIOGET_VSCREENINFO, &var_info);
    var_info.yoffset = 0;
    ioctl(fbfd, FBIOPAN_DISPLAY, &var_info);
}
```

### Implemented in HelixScreen

Added `unblank_display()` and `blank_display()` methods to `DisplayBackendFbdev`:

**Unblank (startup + wake from sleep):**
- Called early in startup (both splash and main app) before `create_display()`
- Called when waking from display sleep timeout
- Uses FBIOBLANK with FB_BLANK_UNBLANK + FBIOPAN_DISPLAY to reset pan

**Blank (sleep timeout):**
- Called when display goes to sleep (brightness 0)
- Uses FBIOBLANK with FB_BLANK_NORMAL

**Files modified:**
- `include/display_backend.h` - virtual methods in base class
- `include/display_backend_fbdev.h` - override declarations
- `src/api/display_backend_fbdev.cpp` - implementation
- `src/application/display_manager.cpp` - calls on startup, sleep, wake
- `src/helix_splash.cpp` - calls on startup

This should eliminate the need for shell script backlight hacks and work
regardless of ForgeX display mode setting.

## Screen Dimming Fix (2026-01-25)

### The Problem
After HelixScreen runs for ~3 seconds, the screen dims to 10% brightness.

**Root cause**: ForgeX's `headless.cfg` has a `reset_screen` delayed_gcode that runs
3 seconds after Klipper starts:

```ini
[delayed_gcode reset_screen]
initial_duration: 3
gcode:
    RUN_SHELL_COMMAND CMD=screen PARAMS='draw_splash'
    _BACKLIGHT S={printer.mod_params.variables.backlight_eco}
```

The `_BACKLIGHT` macro calls `screen.sh backlight {value}`, which sets backlight
to `backlight_eco` (typically 10%).

**Location**: `/data/.mod/.forge-x/root/printer_data/config/mod/macros/headless.cfg`

### The Fix
Patch `/opt/config/mod/.shell/screen.sh` to skip backlight commands when
HelixScreen is active (indicated by `/tmp/helixscreen_active` flag file).

**Patch applied to screen.sh** (in `backlight)` case):
```bash
backlight)
    # Skip if HelixScreen is controlling the display
    if [ -f /tmp/helixscreen_active ]; then
        exit 0
    fi
    value=$2
    ...
```

**Installer handles this automatically**:
- `install.sh` patches screen.sh during ForgeX installation
- `install.sh --uninstall` removes the patch

### Flag File
- `/tmp/helixscreen_active` - created by init script on start, removed on stop
- Tells ForgeX scripts to leave display alone while HelixScreen is running

## Common Issues

1. **Garbled display**: Usually wrong FB format or something drawing over it
2. **Black screen**: Backlight not enabled
3. **ForgeX messages over UI**: S99root's `logged --send-to-screen` output after our init
4. **Screen dims after ~3 seconds**: ForgeX delayed_gcode - see "Screen Dimming Fix" above

## Build Notes

### Cross-Compilation (Docker)
- Uses Colima on macOS (lightweight Docker alternative)
- `make ad5m-docker` - builds using Docker container with ARM toolchain
- `SKIP_OPTIONAL_DEPS=1` passes `--minimal` to check-deps.sh (skips npm/clang-format)
- GCC 10 in Docker requires explicit type for `std::max({...})` - use `std::initializer_list<T>`

### Deployment
- `make deploy-ad5m` - deploys binaries and assets to AD5M
- Uses `scp -O` for legacy SCP protocol (BusyBox has no sftp-server)
- Init script auto-updated if different from deployed version

---

## Backlight Investigation (2026-01-25)

### Current Status
- **GuppyScreen**: Works from cold boot - backlight turns ON
- **HelixScreen**: Does NOT work from cold boot - backlight stays OFF
- Display mode: `GUPPY` in variables.cfg

### What We've Confirmed

1. **FBIOBLANK succeeds** - logs show `Display unblanked via FBIOBLANK`
2. **Allwinner ioctls succeed** - logs show `Allwinner backlight enabled`, `brightness set to 255`
3. **Brightness reads 255** - `backlight.py` with no args returns `255`
4. **But backlight is physically OFF**

### Key Observations

1. ForgeX boot shows splash with backlight ON
2. ~3 seconds after boot completes, backlight turns OFF
3. Our screen.sh patch blocks Klipper's `reset_screen` delayed_gcode (debug log shows `flag=YES`)
4. Something ELSE is turning off the backlight (not screen.sh)
5. Once backlight is off, even manual `backlight.py 100` doesn't turn it back on
6. GuppyScreen somehow CAN turn it on from cold boot
7. When GuppyScreen turns it on and we kill it, backlight STAYS on

### Code Comparison Needed

GuppyScreen source: `~/Code/Printing/guppyscreen`
- `lv_drivers/display/fbdev.c` - their fbdev driver

HelixScreen:
- Uses LVGL 9.4's built-in `lv_linux_fbdev.c`
- Additional code in `src/api/display_backend_fbdev.cpp`

**Key question**: What does GuppyScreen do that we don't?

### GuppyScreen fbdev.c Analysis

```c
void fbdev_init(void) {
    fbfd = open(FBDEV_PATH, O_RDWR);           // 1. Open /dev/fb0
    ioctl(fbfd, FBIOBLANK, FB_BLANK_UNBLANK);  // 2. Unblank IMMEDIATELY
    // ... get screen info, mmap ...
    // NOTE: fd is kept open forever, never closed
}
```

### LVGL 9.4 lv_linux_fbdev.c Analysis

```c
void lv_linux_fbdev_set_file(lv_display_t * disp, const char * file) {
    dsc->fbfd = open(dsc->devname, O_RDWR);    // 1. Open /dev/fb0
    ioctl(dsc->fbfd, FBIOBLANK, FB_BLANK_UNBLANK);  // 2. Unblank
    // ... get screen info, mmap ...
    // NOTE: fd is kept open forever in dsc->fbfd
}
```

**Both do essentially the same thing!** So why does GuppyScreen work?

### Theories to Test

1. **Timing**: GuppyScreen starts at S80, we start at S90. Maybe something happens between?
2. **Init script differences**: GuppyScreen's init is simpler (no flag files, no enable_backlight)
3. **Other processes**: Maybe something in ForgeX startup interferes with later backlight calls
4. **LVGL version**: GuppyScreen uses LVGL 8.x, we use LVGL 9.4 - driver differences?

### Next Steps

1. Compare exact LVGL driver code paths between GuppyScreen (8.x) and HelixScreen (9.4)
2. Check if GuppyScreen has any other backlight-related code we're missing
3. Test starting HelixScreen at S80 instead of S90
4. Check ForgeX source for what runs between S80-S90

### Source Code Locations

- GuppyScreen: `~/Code/Printing/guppyscreen`
- ForgeX: `~/Code/Printing/ad5m-forgex`
- HelixScreen LVGL: `lib/lvgl/src/drivers/display/fb/lv_linux_fbdev.c`
- HelixScreen backend: `src/api/display_backend_fbdev.cpp`

### Key Differences Found

| Aspect | GuppyScreen | HelixScreen |
|--------|-------------|-------------|
| Init script | S80 - just starts binary | S90 - creates flag, calls enable_backlight(), starts binary |
| Boot order | Earlier (S80) | Later (S90) |
| LVGL version | 8.x with custom lv_drivers | 9.4 with built-in linux_fbdev |
| fbdev fd | Global `fbfd`, never closed | Private fd in LVGL driver data |
| enable_backlight() | NOT called | Called via backlight.py before binary |
| Additional ioctls | None (just FBIOBLANK) | Allwinner /dev/disp ioctls |

### ROOT CAUSE FOUND (2026-01-25)

**The screen.sh patch was the problem!**

ForgeX's S99root does this:
```bash
# Turn off and on display's backlight otherwise it's will not work.
"$SCRIPTS"/screen.sh backlight 0
"$SCRIPTS"/screen.sh draw_splash
"$SCRIPTS"/screen.sh backlight 100
```

The backlight 0 → 100 cycle is REQUIRED for the display to work properly.
Our patch blocked this cycle when the flag file existed, breaking the display.

**Solution**: Smart screen.sh patch that allows `backlight 100` but blocks other values.

### Smart screen.sh Patch (2026-01-25)

**Problem**: Need to allow S99root's 0→100 cycle but block Klipper's delayed_gcode dimming.

**Patch applied to `/opt/config/mod/.shell/screen.sh`** (backlight case):
```bash
backlight)
    # Skip non-100 backlight changes when HelixScreen is controlling display
    if [ -f /tmp/helixscreen_active ] && [ "$2" != "100" ]; then
        exit 0
    fi
    value=$2
    ...
```

**Key insight**: Klipper's `_BACKLIGHT` macro uses `RUN_SHELL_COMMAND CMD=screen` which
calls `/root/printer_data/scripts/screen.sh` inside the ForgeX chroot. This is the SAME
file as `/opt/config/mod/.shell/screen.sh` on the host (mounted/linked).

**Debug logging** (temporary, for troubleshooting):
```bash
echo "$(date): backlight $2 called" >> /tmp/backlight_debug.log
if [ -f /tmp/helixscreen_active ]; then
    echo "  flag file EXISTS" >> /tmp/backlight_debug.log
    if [ "$2" != "100" ]; then
        echo "  BLOCKING (value $2 != 100)" >> /tmp/backlight_debug.log
        exit 0
    fi
    echo "  ALLOWING (value is 100)" >> /tmp/backlight_debug.log
else
    echo "  flag file MISSING, allowing" >> /tmp/backlight_debug.log
fi
```

### Delayed Brightness Override (DisplayManager)

Even with the screen.sh patch, we keep a delayed brightness override timer in
`display_manager.cpp` as a safety net:
- Fires 20 seconds after DisplayManager::init()
- Restores configured brightness from SettingsManager
- Uses `spdlog::warn()` so it's visible at default log level

```cpp
lv_timer_create(
    [](lv_timer_t* t) {
        auto* dm = static_cast<DisplayManager*>(lv_timer_get_user_data(t));
        if (dm && dm->m_backlight && dm->m_backlight->is_available()) {
            int brightness = SettingsManager::instance().get_brightness();
            brightness = std::clamp(brightness, 10, 100);
            dm->m_backlight->set_brightness(brightness);
            spdlog::warn("[DisplayManager] Delayed brightness override: {}%", brightness);
        }
        lv_timer_delete(t);
    },
    20000, this);
```

### Display Timeout Settings (2026-01-26)

HelixScreen has TWO separate display timeout settings in `helixconfig.json`:

| Setting | Purpose | Default |
|---------|---------|---------|
| `dim_sec` | Time before screen dims to `dim_brightness` | 600 (10 min) |
| `dim_brightness` | Brightness % when dimmed | 30 |
| `sleep_sec` | Time before screen fully blanks (backlight OFF) | 1800 (30 min) |

**Important**: Both timeouts are independent. If `sleep_sec` < `dim_sec`, the screen goes
straight to sleep without dimming first.

**For debugging**, set both to very high values:
```bash
ssh root@192.168.1.67 'sed -i "s/\"dim_sec\": [0-9]*/\"dim_sec\": 99999/" /opt/helixscreen/config/helixconfig.json'
ssh root@192.168.1.67 'sed -i "s/\"sleep_sec\": [0-9]*/\"sleep_sec\": 99999/" /opt/helixscreen/config/helixconfig.json'
```

### Brightness Inversion Bug (2026-01-26)

**Symptom**: Brightness slider works BACKWARDS - higher values = dimmer, lower values = brighter.
This affects BOTH the HelixScreen slider AND raw `backlight.py` commands.

**Root cause**: Unknown - appears to be Allwinner display driver state corruption. The driver
enters an inverted PWM state. Software reports correct values (90% → 230/255) but physical
brightness is inverted.

**FIX**: Cycle brightness through 0 (disable) then back to desired value:
```bash
/usr/sbin/chroot /data/.mod/.forge-x /root/printer_data/py/backlight.py 0
sleep 1
/usr/sbin/chroot /data/.mod/.forge-x /root/printer_data/py/backlight.py 90
```

**Key findings**:
- Survives soft reboot (`/sbin/reboot`)
- Survives hard power cycle (unplug/replug) - **NOT a hardware state issue**
- Fixed by cycling through brightness 0 (which calls DISP_LCD_BACKLIGHT_DISABLE)
- May be caused by our `unblank_display()` code calling BACKLIGHT_ENABLE + SET_BRIGHTNESS(255)

**Possible cause**: Our code in `display_backend_fbdev.cpp` `unblank_display()` calls:
1. FBIOBLANK (FB_BLANK_UNBLANK)
2. DISP_LCD_BACKLIGHT_ENABLE (0x104)
3. DISP_LCD_SET_BRIGHTNESS (0x102) with value 255

This sequence may put the driver in an inverted state. GuppyScreen does NOT call these
Allwinner ioctls directly - it only uses FBIOBLANK.

**TODO**: Consider removing the Allwinner ioctls from `unblank_display()` and relying only
on `backlight.py` for brightness control.

### Complete Working Sequence (2026-01-26)

**Boot sequence**:
1. Boot starts, ForgeX shows splash with backlight ON
2. HelixScreen init script starts, creates `/tmp/helixscreen_active` flag
3. S99root runs backlight 0→100 cycle:
   - `backlight 0` → **BLOCKED** by smart patch (flag exists, value ≠ 100)
   - `backlight 100` → **ALLOWED** (value = 100)
4. HelixScreen starts, DisplayManager sets backlight to 100%
5. Klipper becomes ready (~15-25s into boot)
6. Klipper's `reset_screen` delayed_gcode fires 3s later:
   - `backlight 10` → **BLOCKED** by smart patch
7. Our 20-second timer fires, sets brightness to user's configured level
8. Display stays bright until `dim_sec` timeout (no touch activity)

**Note**: Timer changed from 30s to 20s (2026-01-26). Klipper typically becomes ready
10-20s after boot, then delayed_gcode fires 3s later. 20s gives enough margin.

### Verification Commands

```bash
# Check current brightness (0-255)
/usr/sbin/chroot /data/.mod/.forge-x /root/printer_data/py/backlight.py

# Check backlight call log (shows what screen.sh blocked/allowed)
cat /tmp/backlight_debug.log

# Check all display settings
grep -E "brightness|dim|sleep" /opt/helixscreen/config/helixconfig.json

# Check if HelixScreen flag exists
ls -la /tmp/helixscreen_active

# Check screen.sh patch
grep -A5 "backlight)" /opt/config/mod/.shell/screen.sh

# Test brightness (should physically brighten if NOT inverted)
/usr/sbin/chroot /data/.mod/.forge-x /root/printer_data/py/backlight.py 90

# Fix inverted brightness
/usr/sbin/chroot /data/.mod/.forge-x /root/printer_data/py/backlight.py 0 && sleep 1 && /usr/sbin/chroot /data/.mod/.forge-x /root/printer_data/py/backlight.py 90
```

### Backlight Control Summary

| Interface | Path | Purpose |
|-----------|------|---------|
| Allwinner ioctl | /dev/disp | SET_BRIGHTNESS (0x102), GET (0x103), ENABLE (0x104), DISABLE (0x105) |
| ForgeX script | screen.sh backlight N | Calls backlight.py inside chroot |
| backlight.py | /root/printer_data/py/backlight.py | Python wrapper for /dev/disp ioctls |
| HelixScreen | BacklightBackendAllwinner | C++ wrapper for /dev/disp ioctls |

**No standard Linux backlight interface** - /sys/class/backlight is empty. Must use /dev/disp ioctls.

### Files Modified for Backlight Fix

**In repository** (committed):
- `src/application/display_manager.cpp` - 20s delayed brightness timer
- `src/api/display_backend_fbdev.cpp` - unblank_display() with Allwinner ioctls
- `scripts/lib/installer/forgex.sh` - smart screen.sh patch function
- `scripts/install-bundled.sh` - same smart patch function

**On AD5M** (per-installation):
- `/opt/config/mod/.shell/screen.sh` - smart patch (blocks non-100 when flag exists)
- `/opt/helixscreen/config/helixconfig.json` - brightness/dim/sleep settings
- `/tmp/helixscreen_active` - flag file (created by init, removed on stop)
- `/tmp/backlight_debug.log` - debug log (if debug logging enabled in screen.sh)
