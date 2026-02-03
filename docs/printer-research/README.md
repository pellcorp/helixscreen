# Printer Research

This directory contains research documentation on various 3D printers for HelixScreen compatibility assessment.

## Overview Matrix

| Printer | Architecture | Display | Moonraker | Multi-Material | HelixScreen Status |
|---------|-------------|---------|-----------|----------------|-------------------|
| **FlashForge AD5M Pro** | ARM (Allwinner T113) | 800x480 FB | Forge-X | None | **Supported** |
| **AnyCubic Kobra S1** | ARM (Cortex-A7) | 480x320 FB | Rinkhals | ACE Pro | **ValgACE ready** |
| **Creality K2 Plus** | MIPS (X2000E?) | 480x800 FB | Stock | CFS (16 colors) | Needs MIPS port |
| **Creality K1C/K1 Max** | MIPS (X2000E) | 480x400 FB | After root | Single | Needs MIPS port |
| **FlashForge AD5X** | MIPS | 720x480 FB | ZMOD | IFS (4 colors) | Needs MIPS port |
| **Snapmaker U1** | Unknown | Unknown | Modified | 4-toolhead | Blocked (closed source) |

## Key Insights

### Already Compatible
- **AD5M Pro**: Fully supported via Forge-X
- **AnyCubic + ACE Pro**: ValgACE backend already implemented, just needs Rinkhals for Moonraker

### Needs MIPS Toolchain
- **Creality K1/K2 series**: All use Ingenic X2000E (MIPS32r2)
- **FlashForge AD5X**: Also MIPS (unlike AD5M which is ARM)
- GuppyScreen proves LVGL works on these - we'd need cross-compilation

### Blocked
- **Snapmaker U1**: Custom Moonraker fork, open source promised by March 2026

## Architecture Quick Reference

### ARM (Current Support)
- Allwinner T113 (AD5M Pro)
- Rockchip RV1106G3 (AnyCubic Kobra series)

### MIPS (Would Need New Toolchain)
- Ingenic X2000E (Creality K1/K2, FlashForge AD5X)
- Dual-core 1.2GHz, 256MB RAM typical
- nan2008 ABI required

## Multi-Material Systems

| System | Type | Colors | Protocol | HelixScreen Support |
|--------|------|--------|----------|---------------------|
| **ACE Pro** | Filament switcher | 4-8 | USB serial + REST | **AmsBackendValgACE** |
| **CFS** | Filament switcher | 4-16 | RS-485 | Via G-code macros |
| **IFS** | Filament switcher | 4 | Serial | Needs investigation |
| **SnapSwap** | Toolchanger | 4 | Unknown | Needs open source |
| **Happy Hare** | MMU | Configurable | Klipper module | **AmsBackendHappyHare** |
| **AFC** | MMU | Configurable | Klipper module | **AmsBackendAFC** |

## Research Documents

| Document | Covers |
|----------|--------|
| [AD5M_BOOT_NOTES.md](AD5M_BOOT_NOTES.md) | AD5M boot process, backlight, ForgeX integration |
| [AD5M_TOOLCHAIN_NOTES.md](AD5M_TOOLCHAIN_NOTES.md) | AD5M cross-compilation, static linking |
| [ANYCUBIC_KOBRA_COREXY_RESEARCH.md](ANYCUBIC_KOBRA_COREXY_RESEARCH.md) | Kobra S1/3 series, ACE Pro, Rinkhals |
| [CREALITY_K1_SERIES_RESEARCH.md](CREALITY_K1_SERIES_RESEARCH.md) | K1C, K1 Max, GuppyScreen |
| [CREALITY_K2_PLUS_RESEARCH.md](CREALITY_K2_PLUS_RESEARCH.md) | K2 Plus, CFS multi-material |
| [FLASHFORGE_AD5X_RESEARCH.md](FLASHFORGE_AD5X_RESEARCH.md) | AD5X, IFS multi-color, ZMOD |
| [SNAPMAKER_U1_RESEARCH.md](SNAPMAKER_U1_RESEARCH.md) | U1 toolchanger, SnapSwap |

## Community Firmware Projects

| Project | Printers | Features |
|---------|----------|----------|
| [Forge-X](https://github.com/DrA1ex/ff5m) | AD5M/Pro | Moonraker, GuppyScreen |
| [ZMOD](https://github.com/ghzserg/zmod) | AD5M/Pro/AD5X | Moonraker, dual-boot |
| [Guilouz Helper](https://github.com/Guilouz/Creality-K1-and-K1-Max) | K1 series | Moonraker, Fluidd |
| [Guppy Mod](https://github.com/ballaswag/creality_k1_klipper_mod) | K1 series | Mainline Klipper |
| [Rinkhals](https://github.com/jbatonnet/Rinkhals) | AnyCubic Kobra | Moonraker overlay |
| [ValgACE](https://github.com/agrloki/ValgACE) | Any Klipper | ACE Pro driver |
