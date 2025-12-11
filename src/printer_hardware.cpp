// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "printer_hardware.h"

#include <spdlog/spdlog.h>

#include <algorithm>

PrinterHardware::PrinterHardware(const std::vector<std::string>& heaters,
                                 const std::vector<std::string>& sensors,
                                 const std::vector<std::string>& fans,
                                 const std::vector<std::string>& leds)
    : heaters_(heaters), sensors_(sensors), fans_(fans), leds_(leds) {}

// ============================================================================
// Static Helpers
// ============================================================================

bool PrinterHardware::has_exact(const std::vector<std::string>& vec, const std::string& name) {
    return std::find(vec.begin(), vec.end(), name) != vec.end();
}

std::string PrinterHardware::find_containing(const std::vector<std::string>& vec,
                                             const std::string& substring) {
    for (const auto& item : vec) {
        if (item.find(substring) != std::string::npos) {
            return item;
        }
    }
    return "";
}

std::string PrinterHardware::find_not_containing(const std::vector<std::string>& vec,
                                                 const std::vector<std::string>& avoid_substrings) {
    for (const auto& item : vec) {
        bool should_avoid = false;
        for (const auto& avoid : avoid_substrings) {
            if (item.find(avoid) != std::string::npos) {
                should_avoid = true;
                break;
            }
        }
        if (!should_avoid) {
            return item;
        }
    }
    return "";
}

// ============================================================================
// Heater Guessing
// ============================================================================

std::string PrinterHardware::guess_bed_heater() const {
    if (heaters_.empty()) {
        spdlog::debug("[PrinterHardware] guess_bed_heater() -> no heaters discovered");
        return "";
    }

    // Priority 1: Exact match for "heater_bed" - Klipper's canonical name
    if (has_exact(heaters_, "heater_bed")) {
        spdlog::debug("[PrinterHardware] guess_bed_heater() -> 'heater_bed'");
        return "heater_bed";
    }

    // Priority 2: Exact match for "heated_bed"
    if (has_exact(heaters_, "heated_bed")) {
        spdlog::debug("[PrinterHardware] guess_bed_heater() -> 'heated_bed'");
        return "heated_bed";
    }

    // Priority 3: Any heater containing "bed"
    std::string match = find_containing(heaters_, "bed");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_bed_heater() -> '{}'", match);
        return match;
    }

    spdlog::debug("[PrinterHardware] guess_bed_heater() -> no match found");
    return "";
}

std::string PrinterHardware::guess_hotend_heater() const {
    if (heaters_.empty()) {
        spdlog::debug("[PrinterHardware] guess_hotend_heater() -> no heaters discovered");
        return "";
    }

    // Priority 1: Exact match for "extruder" - Klipper's canonical [extruder] section
    if (has_exact(heaters_, "extruder")) {
        spdlog::debug("[PrinterHardware] guess_hotend_heater() -> 'extruder'");
        return "extruder";
    }

    // Priority 2: Exact match for "extruder0"
    if (has_exact(heaters_, "extruder0")) {
        spdlog::debug("[PrinterHardware] guess_hotend_heater() -> 'extruder0'");
        return "extruder0";
    }

    // Priority 3: Any heater containing "extruder"
    std::string match = find_containing(heaters_, "extruder");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_hotend_heater() -> '{}'", match);
        return match;
    }

    // Priority 4: Any heater containing "hotend"
    match = find_containing(heaters_, "hotend");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_hotend_heater() -> '{}'", match);
        return match;
    }

    // Priority 5: Any heater containing "e0"
    match = find_containing(heaters_, "e0");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_hotend_heater() -> '{}'", match);
        return match;
    }

    spdlog::debug("[PrinterHardware] guess_hotend_heater() -> no match found");
    return "";
}

// ============================================================================
// Sensor Guessing
// ============================================================================

std::string PrinterHardware::guess_bed_sensor() const {
    // First check heaters - heaters have built-in sensors
    std::string bed_heater = guess_bed_heater();
    if (!bed_heater.empty()) {
        spdlog::debug("[PrinterHardware] guess_bed_sensor() -> '{}' (from heater)", bed_heater);
        return bed_heater;
    }

    // Search sensors for bed-related names
    std::string match = find_containing(sensors_, "bed");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_bed_sensor() -> '{}'", match);
        return match;
    }

    spdlog::debug("[PrinterHardware] guess_bed_sensor() -> no match found");
    return "";
}

std::string PrinterHardware::guess_hotend_sensor() const {
    // First check heaters - heaters have built-in sensors
    std::string hotend_heater = guess_hotend_heater();
    if (!hotend_heater.empty()) {
        spdlog::debug("[PrinterHardware] guess_hotend_sensor() -> '{}' (from heater)",
                      hotend_heater);
        return hotend_heater;
    }

    // Search sensors for extruder/hotend-related names
    std::string match = find_containing(sensors_, "extruder");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_hotend_sensor() -> '{}'", match);
        return match;
    }

    match = find_containing(sensors_, "hotend");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_hotend_sensor() -> '{}'", match);
        return match;
    }

    match = find_containing(sensors_, "e0");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_hotend_sensor() -> '{}'", match);
        return match;
    }

    spdlog::debug("[PrinterHardware] guess_hotend_sensor() -> no match found");
    return "";
}

// ============================================================================
// Fan Guessing
// ============================================================================

std::string PrinterHardware::guess_part_cooling_fan() const {
    if (fans_.empty()) {
        spdlog::debug("[PrinterHardware] guess_part_cooling_fan() -> no fans discovered");
        return "";
    }

    // Priority 1: Exact match for "fan" - Klipper's canonical [fan] section
    // This is THE part cooling fan, controlled by M106/M107
    if (has_exact(fans_, "fan")) {
        spdlog::debug("[PrinterHardware] guess_part_cooling_fan() -> 'fan' (canonical)");
        return "fan";
    }

    // Priority 2: Any fan containing "part" (e.g., "fan_generic part_cooling")
    std::string match = find_containing(fans_, "part");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_part_cooling_fan() -> '{}' (contains 'part')",
                      match);
        return match;
    }

    // Fallback: first fan in list (better than nothing)
    spdlog::debug("[PrinterHardware] guess_part_cooling_fan() -> '{}' (fallback)", fans_[0]);
    return fans_[0];
}

// ============================================================================
// LED Guessing
// ============================================================================

std::string PrinterHardware::guess_main_led_strip() const {
    if (leds_.empty()) {
        spdlog::debug("[PrinterHardware] guess_main_led_strip() -> no LEDs discovered");
        return "";
    }

    // Priority 1: LEDs containing "case" (most common for case lighting)
    std::string match = find_containing(leds_, "case");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_main_led_strip() -> '{}' (contains 'case')", match);
        return match;
    }

    // Priority 2: LEDs containing "chamber"
    match = find_containing(leds_, "chamber");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_main_led_strip() -> '{}' (contains 'chamber')",
                      match);
        return match;
    }

    // Priority 3: LEDs containing "light" (general lighting)
    match = find_containing(leds_, "light");
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_main_led_strip() -> '{}' (contains 'light')", match);
        return match;
    }

    // Priority 4: Any LED that's NOT a specialty indicator
    // Avoid: indicator, status, corner (these are typically status LEDs, not room lighting)
    match = find_not_containing(leds_,
                                {"indicator", "status", "corner", "Indicator", "Status", "Corner"});
    if (!match.empty()) {
        spdlog::debug("[PrinterHardware] guess_main_led_strip() -> '{}' (non-indicator)", match);
        return match;
    }

    // Fallback: first LED in list
    spdlog::debug("[PrinterHardware] guess_main_led_strip() -> '{}' (fallback)", leds_[0]);
    return leds_[0];
}
