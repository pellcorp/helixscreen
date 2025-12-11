// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

/**
 * @file printer_hardware.h
 * @brief Hardware discovery heuristics for Klipper printers
 *
 * This class encapsulates all the Klipper naming convention knowledge needed
 * to intelligently guess which hardware components serve which purpose.
 * It operates on raw hardware lists (from MoonrakerClient) and applies
 * priority-based heuristics to find the most likely matches.
 *
 * ## Design Rationale
 *
 * Hardware guessing logic was extracted from MoonrakerClient/MoonrakerAPI to:
 * - Keep protocol/connection code separate from business logic
 * - Centralize Klipper naming convention knowledge in one place
 * - Make heuristics testable in isolation
 * - Allow future extension with printer-specific profiles
 *
 * ## Usage
 *
 * ```cpp
 * PrinterHardware hw(client.get_heaters(), client.get_sensors(),
 *                    client.get_fans(), client.get_leds());
 *
 * std::string bed = hw.guess_bed_heater();       // e.g., "heater_bed"
 * std::string fan = hw.guess_part_cooling_fan(); // e.g., "fan"
 * ```
 */
class PrinterHardware {
  public:
    /**
     * @brief Construct with hardware lists from MoonrakerClient
     *
     * Takes const references to avoid copying large vectors.
     * The caller must ensure the vectors remain valid during PrinterHardware's lifetime.
     *
     * @param heaters Controllable heaters (extruders, bed, generic heaters)
     * @param sensors Read-only temperature sensors
     * @param fans All fan types (part cooling, bed fans, exhaust, etc.)
     * @param leds LED outputs (neopixel, dotstar, led, pca9632)
     */
    PrinterHardware(const std::vector<std::string>& heaters,
                    const std::vector<std::string>& sensors, const std::vector<std::string>& fans,
                    const std::vector<std::string>& leds);

    // ========================================================================
    // Heater Guessing
    // ========================================================================

    /**
     * @brief Guess the most likely bed heater
     *
     * Priority order:
     * 1. Exact match: "heater_bed" (Klipper's canonical name)
     * 2. Exact match: "heated_bed"
     * 3. Substring match: any heater containing "bed"
     *
     * @return Bed heater name or empty string if none found
     */
    std::string guess_bed_heater() const;

    /**
     * @brief Guess the most likely hotend heater
     *
     * Priority order:
     * 1. Exact match: "extruder" (Klipper's canonical [extruder] section)
     * 2. Exact match: "extruder0"
     * 3. Substring match: any heater containing "extruder"
     * 4. Substring match: any heater containing "hotend"
     * 5. Substring match: any heater containing "e0"
     *
     * @return Hotend heater name or empty string if none found
     */
    std::string guess_hotend_heater() const;

    // ========================================================================
    // Sensor Guessing
    // ========================================================================

    /**
     * @brief Guess the most likely bed temperature sensor
     *
     * First checks heaters for bed heater (heaters have built-in sensors).
     * If no bed heater found, searches sensors for names containing "bed".
     *
     * @return Bed sensor name or empty string if none found
     */
    std::string guess_bed_sensor() const;

    /**
     * @brief Guess the most likely hotend temperature sensor
     *
     * First checks heaters for extruder heater (heaters have built-in sensors).
     * If no extruder heater found, searches sensors for names containing
     * "extruder", "hotend", "e0".
     *
     * @return Hotend sensor name or empty string if none found
     */
    std::string guess_hotend_sensor() const;

    // ========================================================================
    // Fan Guessing
    // ========================================================================

    /**
     * @brief Guess the most likely part cooling fan
     *
     * In Klipper, the [fan] section (without any suffix) is the canonical
     * part cooling fan controlled by M106/M107 G-code commands.
     *
     * Priority order:
     * 1. Exact match: "fan" (Klipper's canonical [fan] section)
     * 2. Substring match: any fan containing "part" (e.g., "fan_generic part_cooling")
     * 3. Fallback: first fan in list (if no better match)
     *
     * Avoids auxiliary fans (bed_fans, exhaust, nevermore, controller_fan)
     * by prioritizing the canonical "fan" name.
     *
     * @return Part cooling fan name or empty string if none found
     */
    std::string guess_part_cooling_fan() const;

    // ========================================================================
    // LED Guessing
    // ========================================================================

    /**
     * @brief Guess the most likely main LED strip (case/chamber lighting)
     *
     * Priority order:
     * 1. Substring match: "case" (e.g., "neopixel case_lights")
     * 2. Substring match: "chamber" (e.g., "neopixel chamber_light")
     * 3. Substring match: "light" (e.g., "led toolhead_light")
     * 4. Avoid specialty indicators: skip LEDs containing "indicator", "status", "corner"
     * 5. Fallback: first LED in list (if no better match)
     *
     * The goal is to find the primary case/chamber lighting that users
     * typically want to control, rather than status LEDs or indicators.
     *
     * @return Main LED strip name or empty string if none found
     */
    std::string guess_main_led_strip() const;

  private:
    const std::vector<std::string>& heaters_;
    const std::vector<std::string>& sensors_;
    const std::vector<std::string>& fans_;
    const std::vector<std::string>& leds_;

    /**
     * @brief Helper: find exact match in a vector
     */
    static bool has_exact(const std::vector<std::string>& vec, const std::string& name);

    /**
     * @brief Helper: find first item containing substring
     */
    static std::string find_containing(const std::vector<std::string>& vec,
                                       const std::string& substring);

    /**
     * @brief Helper: find first item NOT containing any of the given substrings
     */
    static std::string find_not_containing(const std::vector<std::string>& vec,
                                           const std::vector<std::string>& avoid_substrings);
};
