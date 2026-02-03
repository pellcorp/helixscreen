# Creality K2 Plus (and K2 Series) Research

**Date**: 2026-02-02
**Status**: Comprehensive research complete

## Executive Summary

The Creality K2 Plus is Creality's flagship CoreXY enclosed printer with 350mm³ build volume. It runs **Creality OS** (modified Klipper) and supports multi-material via **CFS (Creality Filament System)** - up to 16 colors with 4 daisy-chained units. **Moonraker is included in stock firmware** on port 4408. The K2 Plus likely uses the same Ingenic X2000E MIPS processor as the K1 series.

---

## 1. Hardware Specifications

### K2 Plus

| Specification | Details |
|---------------|---------|
| **Build Volume** | 350 x 350 x 350 mm |
| **Max Print Speed** | 600 mm/s |
| **Max Acceleration** | 30,000 mm/s² |
| **Nozzle Temperature** | Up to 350°C |
| **Bed Temperature** | Up to 120°C |
| **Chamber Temperature** | Up to 60°C (actively heated) |
| **Display** | 4.3" LCD touchscreen, 480 x 800 |
| **Storage** | 32 GB onboard |
| **Connectivity** | Dual-band WiFi, Ethernet |
| **Motion** | Step-servo motors, 32,768 microsteps/rev |
| **Weight** | 35 kg |
| **Price** | $1,499 (Combo with CFS) |

### Processor (Likely)
Based on K1 discovery research:
- **CPU**: Ingenic X2000E SoC (MIPS, dual-core 1.2GHz)
- **RAM**: LPDDR2 (unconfirmed, likely 512MB-1GB)
- **MCU**: GD32F303RET6 (ARM Cortex-M3) for motion

### K2 Series Variants

| Model | Build Volume | Chamber Heater | Price |
|-------|-------------|----------------|-------|
| **K2** | 260³ mm | No | $549-699 |
| **K2 Pro** | 300³ mm | Yes (60°C) | $849-1,049 |
| **K2 Plus** | 350³ mm | Yes (60°C) | $1,199-1,499 |
| **K2 SE** | 220x215x245 mm | No | Lower cost |

---

## 2. Stock Firmware

### Operating System
- **Tina Linux 21.02-SNAPSHOT** (Buildroot-based)
- **Kernel**: Linux 4.x
- **Klipper**: Custom fork with proprietary extensions
- **Python**: 3.9

### Firmware Distribution
- Updates as `.img` files
- CFS firmware as `.bin` files
- OTA via Creality Cloud
- Extracted at [Guilouz/Creality-K2Plus-Extracted-Firmwares](https://github.com/Guilouz/Creality-K2Plus-Extracted-Firmwares)

---

## 3. Multi-Material System (CFS)

The **Creality Filament System** is Creality's answer to Bambu's AMS.

### CFS Specifications
| Feature | Value |
|---------|-------|
| **Spool Capacity** | 4 per unit |
| **Max Units** | 4 (daisy-chained) |
| **Max Colors** | 16 |
| **Communication** | RS-485 protocol |
| **Features** | Humidity/temp monitoring, RFID, auto-backup |

### CFS Communication
- RS-485 serial via dedicated cables
- Proprietary Python wrappers (compiled `.so` blobs):
  - `filament_rack_wrapper.cpython-39.so`
  - `serial_485_wrapper.cpython-39.so`
  - `box_wrapper.cpython-39.so`

### CFS G-code Macros
```
BOX_GO_TO_EXTRUDE_POS
BOX_NOZZLE_CLEAN
BOX_MOVE_TO_SAFE_POS
BOX_CUT_MATERIAL
T0, T1, T2, T3 (tool change)
```

---

## 4. Moonraker Availability

### Stock: YES (unlike K1 series)

| Service | Port |
|---------|------|
| Fluidd | 4408 |
| Mainsail | 4409 |
| Moonraker API | 4408 |

### Limitations
- Some features restricted in stock
- Full functionality after rooting
- CFS mapping unavailable via native OctoPrint/Klipper file transfer

---

## 5. Custom Firmware Options

### Official Open Source
**Repository**: [CrealityOfficial/K2_Series_Klipper](https://github.com/CrealityOfficial/K2_Series_Klipper)

Community criticism:
- No build documentation
- Outdated Tina Linux base (5 years old)
- CFS modules only as compiled blobs
- No integration instructions

### Community Projects

| Project | Description |
|---------|-------------|
| [k2-improvements](https://github.com/jamincollins/k2-improvements) | Full Klipper venv, Entware (archived) |
| [Fluidd-K2](https://github.com/BusPirateV5/Fluidd-K2) | Customized Fluidd, WebRTC camera |
| [Mainsail-K2](https://github.com/Guilouz/Mainsail-K2) | Lightweight Mainsail build |
| [k2_powerups](https://github.com/minimal3dp/k2_powerups) | Improved leveling/start procedures |

---

## 6. Klipper Configuration Structure

### Location
`/usr/share/klipper/config/`:
- `printer.cfg` - Main config
- `gcode_macro.cfg` - G-code macros

### Key Variables
```ini
z_safe_g28: 0.0
max_x_position: 350
max_y_position: 352
max_z_position: 320
```

### Notable Features
- `Qmode` - Quiet mode (2500mm/s² accel, 150mm/s velocity)
- Chamber heater control (`M141`, `M191`)
- Input shaper calibration
- `BOX_*` macros for CFS

### Closed-Source Components
- `box_wrapper.cpython-39.so`
- `filament_rack_wrapper.cpython-39.so`
- `serial_485_wrapper.cpython-39.so`
- Master server/display application

---

## 7. Display Interface

### Hardware
- **Size**: 4.3 inches
- **Resolution**: 480 x 800 pixels
- **Type**: LCD touchscreen
- **Interface**: `/dev/fb0`

### Software
- Stock UI likely LVGL-based (similar to K1)
- Direct framebuffer rendering
- No X11/Wayland

---

## 8. Root Access

### Enabling Root
1. Settings > "Root account information"
2. Read disclaimer, check acknowledgment
3. Wait 30 seconds, press "Ok"
4. SSH credentials displayed

### Credentials
- **Username**: `root`
- **Password**: `creality_2024`

```bash
ssh root@<printer-ip>
```

---

## 9. HelixScreen Compatibility Assessment

### Favorable Factors
1. **Moonraker included** - Stock firmware has Moonraker on port 4408
2. **Root access available** - SSH with default credentials
3. **Linux framebuffer** - Direct `/dev/fb0` access
4. **LVGL precedent** - Stock UI and GuppyScreen both use LVGL

### Challenges

| Challenge | Severity | Notes |
|-----------|----------|-------|
| MIPS Architecture | HIGH | Requires MIPS32r2 cross-compilation |
| Display Resolution | MEDIUM | 480x800 portrait orientation |
| CFS Integration | MEDIUM | Proprietary blobs for filament system |
| Resource Constraints | MEDIUM | Limited CPU/RAM |

### Implementation Path
1. **Confirm SoC** - Check `/proc/cpuinfo`
2. **Build toolchain** - Cross-compilation for MIPS
3. **Port LVGL drivers** - Framebuffer compatibility
4. **Moonraker integration** - Use existing API
5. **CFS support** - Use G-code macros (T0-T3, BOX_*)

---

## 10. Community Resources

### Official
- **Forum**: [forum.creality.com/c/flagship-series/creality-flagship-k2-plus/81](https://forum.creality.com/c/flagship-series/creality-flagship-k2-plus/81)
- **Wiki**: [wiki.creality.com/en/k2-flagship-series/k2-plus](https://wiki.creality.com/en/k2-flagship-series/k2-plus)

### Discord
- [discord.com/invite/creality](https://discord.com/invite/creality)

### GitHub
| Repository | Purpose |
|------------|---------|
| [CrealityOfficial/K2_Series_Klipper](https://github.com/CrealityOfficial/K2_Series_Klipper) | Official Klipper fork |
| [Guilouz/Creality-K2Plus-Extracted-Firmwares](https://github.com/Guilouz/Creality-K2Plus-Extracted-Firmwares) | Extracted firmware |
| [ballaswag/guppyscreen](https://github.com/ballaswag/guppyscreen) | GuppyScreen (K2 not yet supported) |

---

## Conclusion

The Creality K2 Plus is a capable HelixScreen target with stock Moonraker support. Key challenges:
1. **Architecture** - Likely MIPS requiring specialized cross-compilation
2. **CFS integration** - Multi-material via existing macros or reverse-engineering
3. **Display takeover** - Must handle stock display-server
4. **Community tooling** - K2 Plus scene less mature than K1 series

Most promising path: Study GuppyScreen's LVGL/Moonraker architecture and adapt, while using existing CFS G-code macros.
