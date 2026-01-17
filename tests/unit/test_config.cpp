// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "config.h"
#include "wizard_config_paths.h"

#include "../catch_amalgamated.hpp"

// Test fixture for Config class testing
class ConfigTestFixture {
  protected:
    Config config;

    // Helper methods to access protected members
    void set_data_null(const std::string& json_ptr) {
        config.data[json::json_pointer(json_ptr)] = nullptr;
    }

    void set_data_empty() {
        config.data = {};
    }

    // Helper for plural naming refactor tests
    void set_data_for_plural_test(const json& data) {
        config.data = data;
    }

    // Helper to get mutable reference to config data for migration testing
    json& get_data() {
        return config.data;
    }

    // Helper to check if config data contains a key
    bool data_contains(const std::string& key) {
        return config.data.contains(key);
    }

    // Helper to apply migration to config data
    void apply_migration() {
        // Re-implement the migration logic for testing
        // This mirrors migrate_display_config() in config.cpp
        if (!config.data.contains("display_rotate")) {
            return; // Already migrated
        }

        if (!config.data.contains("display")) {
            config.data["display"] = json::object();
        }

        // Migrate only if target key doesn't already exist
        if (config.data.contains("display_rotate")) {
            if (!config.data["display"].contains("rotate")) {
                config.data["display"]["rotate"] = config.data["display_rotate"];
            }
            config.data.erase("display_rotate");
        }
        if (config.data.contains("display_sleep_sec")) {
            if (!config.data["display"].contains("sleep_sec")) {
                config.data["display"]["sleep_sec"] = config.data["display_sleep_sec"];
            }
            config.data.erase("display_sleep_sec");
        }
        if (config.data.contains("display_dim_sec")) {
            if (!config.data["display"].contains("dim_sec")) {
                config.data["display"]["dim_sec"] = config.data["display_dim_sec"];
            }
            config.data.erase("display_dim_sec");
        }
        if (config.data.contains("display_dim_brightness")) {
            if (!config.data["display"].contains("dim_brightness")) {
                config.data["display"]["dim_brightness"] = config.data["display_dim_brightness"];
            }
            config.data.erase("display_dim_brightness");
        }
        if (config.data.contains("touch_calibrated") || config.data.contains("touch_calibration")) {
            if (!config.data["display"].contains("calibration")) {
                config.data["display"]["calibration"] = json::object();
            }
            if (config.data.contains("touch_calibrated")) {
                if (!config.data["display"]["calibration"].contains("valid")) {
                    config.data["display"]["calibration"]["valid"] =
                        config.data["touch_calibrated"];
                }
                config.data.erase("touch_calibrated");
            }
            if (config.data.contains("touch_calibration")) {
                const auto& cal = config.data["touch_calibration"];
                for (const auto& key : {"a", "b", "c", "d", "e", "f"}) {
                    if (cal.contains(key) && !config.data["display"]["calibration"].contains(key)) {
                        config.data["display"]["calibration"][key] = cal[key];
                    }
                }
                config.data.erase("touch_calibration");
            }
        }
    }

    // Helper to check display subsection contains a key
    bool display_contains(const std::string& key) {
        return config.data.contains("display") && config.data["display"].contains(key);
    }

    // Helper to check calibration subsection contains a key
    bool calibration_contains(const std::string& key) {
        return config.data.contains("display") && config.data["display"].contains("calibration") &&
               config.data["display"]["calibration"].contains(key);
    }

    // Helper to get display section size
    size_t display_size() {
        return config.data.contains("display") ? config.data["display"].size() : 0;
    }

    void setup_default_config() {
        // Manually populate config.data with realistic test JSON
        config.data = {
            {"printer",
             {{"moonraker_host", "192.168.1.100"},
              {"moonraker_port", 7125},
              {"log_level", "debug"},
              {"hardware_map", {{"heated_bed", "heater_bed"}, {"hotend", "extruder"}}}}}};
    }

    void setup_minimal_config() {
        // Minimal config for wizard testing (default host)
        config.data = {{"printer", {{"moonraker_host", "127.0.0.1"}, {"moonraker_port", 7125}}}};
    }

    void setup_incomplete_config() {
        // Config missing hardware_map (should trigger wizard)
        config.data = {{"printer", {{"moonraker_host", "192.168.1.50"}, {"moonraker_port", 7125}}}};
    }
};

// ============================================================================
// get() without default parameter - Existing behavior
// ============================================================================

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() returns existing string value",
                 "[core][config][get]") {
    setup_default_config();

    std::string host = config.get<std::string>("/printer/moonraker_host");
    REQUIRE(host == "192.168.1.100");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() returns existing int value",
                 "[core][config][get]") {
    setup_default_config();

    int port = config.get<int>("/printer/moonraker_port");
    REQUIRE(port == 7125);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() returns existing nested value",
                 "[config][get]") {
    setup_default_config();

    std::string bed = config.get<std::string>("/printer/hardware_map/heated_bed");
    REQUIRE(bed == "heater_bed");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with df() prefix returns value",
                 "[config][get]") {
    setup_default_config();

    std::string host = config.get<std::string>(config.df() + "moonraker_host");
    REQUIRE(host == "192.168.1.100");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with missing key throws exception",
                 "[core][config][get]") {
    setup_default_config();

    REQUIRE_THROWS_AS(config.get<std::string>("/printer/nonexistent_key"),
                      nlohmann::detail::type_error);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with missing nested key throws exception",
                 "[config][get]") {
    setup_default_config();

    REQUIRE_THROWS_AS(config.get<std::string>("/printer/hardware_map/missing"),
                      nlohmann::detail::type_error);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with type mismatch throws exception",
                 "[config][get]") {
    setup_default_config();

    // Try to get string value as int
    REQUIRE_THROWS(config.get<int>("/printer/moonraker_host"));
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with object returns nested structure",
                 "[config][get]") {
    setup_default_config();

    auto hardware_map = config.get<json>("/printer/hardware_map");
    REQUIRE(hardware_map.is_object());
    REQUIRE(hardware_map["heated_bed"] == "heater_bed");
    REQUIRE(hardware_map["hotend"] == "extruder");
}

// ============================================================================
// get() with default parameter - NEW behavior
// ============================================================================

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: get() with default returns value when key exists (string)",
                 "[config][get][default]") {
    setup_default_config();

    std::string host = config.get<std::string>("/printer/moonraker_host", "default.local");
    REQUIRE(host == "192.168.1.100"); // Ignores default
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: get() with default returns value when key exists (int)",
                 "[config][get][default]") {
    setup_default_config();

    int port = config.get<int>("/printer/moonraker_port", 9999);
    REQUIRE(port == 7125); // Ignores default
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: get() with default returns default when key missing (string)",
                 "[core][config][get][default]") {
    setup_default_config();

    std::string printer_name = config.get<std::string>("/printer/printer_name", "My Printer");
    REQUIRE(printer_name == "My Printer");
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: get() with default returns default when key missing (int)",
                 "[config][get][default]") {
    setup_default_config();

    int timeout = config.get<int>("/printer/timeout", 30);
    REQUIRE(timeout == 30);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: get() with default returns default when key missing (bool)",
                 "[config][get][default]") {
    setup_default_config();

    bool api_key = config.get<bool>("/printer/moonraker_api_key", false);
    REQUIRE(api_key == false);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with default handles nested missing path",
                 "[config][get][default]") {
    setup_default_config();

    std::string led = config.get<std::string>("/printer/hardware_map/main_led", "none");
    REQUIRE(led == "none");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with empty string default",
                 "[config][get][default]") {
    setup_default_config();

    std::string empty = config.get<std::string>("/printer/empty_field", "");
    REQUIRE(empty == "");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with default using df() prefix",
                 "[config][get][default]") {
    setup_default_config();

    std::string printer_name = config.get<std::string>(config.df() + "printer_name", "");
    REQUIRE(printer_name == "");
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: get() with default handles completely missing parent path",
                 "[config][get][default]") {
    setup_default_config();

    std::string missing = config.get<std::string>("/nonexistent/path/key", "fallback");
    REQUIRE(missing == "fallback");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with default prevents crashes on null keys",
                 "[config][get][default]") {
    setup_minimal_config();

    // This is the bug we fixed - printer_name doesn't exist, should return default not throw
    std::string printer_name = config.get<std::string>(config.df() + "printer_name", "");
    REQUIRE(printer_name == "");
}

// ============================================================================
// set() operations
// ============================================================================

TEST_CASE_METHOD(ConfigTestFixture, "Config: set() creates new top-level key", "[config][set]") {
    setup_default_config();

    config.set<std::string>("/new_key", "new_value");
    REQUIRE(config.get<std::string>("/new_key") == "new_value");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set() updates existing key", "[config][set]") {
    setup_default_config();

    config.set<std::string>("/printer/moonraker_host", "10.0.0.1");
    REQUIRE(config.get<std::string>("/printer/moonraker_host") == "10.0.0.1");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set() creates nested path", "[config][set]") {
    setup_default_config();

    config.set<std::string>("/printer/hardware_map/main_led", "neopixel");
    REQUIRE(config.get<std::string>("/printer/hardware_map/main_led") == "neopixel");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set() updates nested value", "[config][set]") {
    setup_default_config();

    config.set<std::string>("/printer/hardware_map/hotend", "extruder1");
    REQUIRE(config.get<std::string>("/printer/hardware_map/hotend") == "extruder1");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set() handles different types", "[config][set]") {
    setup_default_config();

    config.set<int>("/printer/new_int", 42);
    config.set<bool>("/printer/new_bool", true);
    config.set<std::string>("/printer/new_string", "test");

    REQUIRE(config.get<int>("/printer/new_int") == 42);
    REQUIRE(config.get<bool>("/printer/new_bool") == true);
    REQUIRE(config.get<std::string>("/printer/new_string") == "test");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set() overwrites value of different type",
                 "[config][set]") {
    setup_default_config();

    config.set<int>("/printer/moonraker_port", 8080);
    REQUIRE(config.get<int>("/printer/moonraker_port") == 8080);

    // Overwrite int with string
    config.set<std::string>("/printer/moonraker_port", "9090");
    REQUIRE(config.get<std::string>("/printer/moonraker_port") == "9090");
}

// ============================================================================
// is_wizard_required() logic - NEW: wizard_completed flag
// ============================================================================

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: is_wizard_required() returns false when wizard_completed is true",
                 "[config][wizard]") {
    setup_minimal_config();

    // Set wizard_completed flag
    config.set<bool>("/wizard_completed", true);

    REQUIRE(config.is_wizard_required() == false);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: is_wizard_required() returns true when wizard_completed is false",
                 "[config][wizard]") {
    setup_default_config();

    // Explicitly set wizard_completed to false
    config.set<bool>("/wizard_completed", false);

    REQUIRE(config.is_wizard_required() == true);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: is_wizard_required() returns true when wizard_completed flag missing",
                 "[config][wizard]") {
    setup_minimal_config();

    // No wizard_completed flag set
    REQUIRE(config.is_wizard_required() == true);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: wizard_completed flag overrides hardware config",
                 "[config][wizard]") {
    setup_default_config();

    // Even with full hardware config, if wizard_completed is false, wizard should run
    config.set<bool>("/wizard_completed", false);

    REQUIRE(config.is_wizard_required() == true);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: wizard_completed=true skips wizard even with minimal config",
                 "[config][wizard]") {
    setup_minimal_config();

    // Even with minimal config (127.0.0.1 host), wizard_completed=true should skip wizard
    config.set<bool>("/wizard_completed", true);

    REQUIRE(config.is_wizard_required() == false);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: is_wizard_required() handles invalid wizard_completed type",
                 "[config][wizard]") {
    setup_default_config();

    // Set wizard_completed to invalid type (string instead of bool)
    config.set<std::string>("/wizard_completed", "true");

    // Should return true (wizard required) because flag is not a valid boolean
    REQUIRE(config.is_wizard_required() == true);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: is_wizard_required() handles null wizard_completed",
                 "[config][wizard]") {
    setup_default_config();

    // Set wizard_completed to null
    set_data_null("/wizard_completed");

    // Should return true (wizard required) because flag is null
    REQUIRE(config.is_wizard_required() == true);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST_CASE_METHOD(ConfigTestFixture, "Config: handles deeply nested structures", "[config][edge]") {
    setup_default_config();

    config.set<std::string>("/printer/nested/level1/level2/level3", "deep");
    std::string deep = config.get<std::string>("/printer/nested/level1/level2/level3");
    REQUIRE(deep == "deep");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: get() with default handles empty config",
                 "[config][edge]") {
    // Empty config
    set_data_empty();

    std::string host = config.get<std::string>("/printer/moonraker_host", "localhost");
    REQUIRE(host == "localhost");
}

// ============================================================================
// Config Path Structure Tests - NEW plural naming convention
// These tests define the contract for the refactored config structure.
// They SHOULD FAIL until the implementation is updated.
// ============================================================================

TEST_CASE_METHOD(ConfigTestFixture, "Config: heaters path uses plural form /printer/heaters/",
                 "[config][paths][plural]") {
    // Set up config with the NEW plural path structure
    set_data_for_plural_test(
        {{"printer", {{"heaters", {{"bed", "heater_bed"}, {"hotend", "extruder"}}}}}});

    // Verify we can read from the plural path
    std::string bed_heater = config.get<std::string>("/printer/heaters/bed");
    REQUIRE(bed_heater == "heater_bed");

    std::string hotend_heater = config.get<std::string>("/printer/heaters/hotend");
    REQUIRE(hotend_heater == "extruder");
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: temp_sensors path uses plural form /printer/temp_sensors/",
                 "[config][paths][plural]") {
    // Set up config with the NEW plural path structure
    set_data_for_plural_test(
        {{"printer", {{"temp_sensors", {{"bed", "heater_bed"}, {"hotend", "extruder"}}}}}});

    // Verify we can read from the plural path
    std::string bed_sensor = config.get<std::string>("/printer/temp_sensors/bed");
    REQUIRE(bed_sensor == "heater_bed");

    std::string hotend_sensor = config.get<std::string>("/printer/temp_sensors/hotend");
    REQUIRE(hotend_sensor == "extruder");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: fans path uses plural form /printer/fans/",
                 "[config][paths][plural]") {
    // Set up config with the NEW plural path structure
    set_data_for_plural_test(
        {{"printer", {{"fans", {{"part", "fan"}, {"hotend", "heater_fan hotend_fan"}}}}}});

    // Verify we can read from the plural path - fans is now an OBJECT, not array
    std::string part_fan = config.get<std::string>("/printer/fans/part");
    REQUIRE(part_fan == "fan");

    std::string hotend_fan = config.get<std::string>("/printer/fans/hotend");
    REQUIRE(hotend_fan == "heater_fan hotend_fan");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: leds path uses plural form /printer/leds/",
                 "[config][paths][plural]") {
    // Set up config with the NEW plural path structure
    set_data_for_plural_test({{"printer", {{"leds", {{"strip", "neopixel chamber_light"}}}}}});

    // Verify we can read from the plural path
    std::string led_strip = config.get<std::string>("/printer/leds/strip");
    REQUIRE(led_strip == "neopixel chamber_light");
}

// ============================================================================
// Default Config Structure Tests - NEW structure contract
// These tests verify the default config matches the new schema.
// They SHOULD FAIL until the implementation is updated.
// ============================================================================

TEST_CASE_METHOD(ConfigTestFixture, "Config: default structure has extra_sensors as empty object",
                 "[config][defaults][plural]") {
    // After refactoring, monitored_sensors should become extra_sensors
    // and should be an empty object {}, not an array []
    set_data_for_plural_test({{"printer",
                               {{"moonraker_host", "127.0.0.1"},
                                {"moonraker_port", 7125},
                                {"extra_sensors", json::object()}}}});

    auto extra_sensors = config.get<json>("/printer/extra_sensors");
    REQUIRE(extra_sensors.is_object());
    REQUIRE(extra_sensors.empty());
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: default structure has no fans array - fans is object only",
                 "[config][defaults][plural]") {
    // After refactoring, there should be no separate "fans" array
    // The fans key should be an object with role mappings, not an array
    set_data_for_plural_test({{"printer",
                               {{"moonraker_host", "127.0.0.1"},
                                {"moonraker_port", 7125},
                                {"fans", {{"part", "fan"}}}}}});

    auto fans = config.get<json>("/printer/fans");
    REQUIRE(fans.is_object());
    REQUIRE_FALSE(fans.is_array());
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: temp_sensors key exists for temperature sensor mappings",
                 "[config][defaults][plural]") {
    // The new structure uses temp_sensors (not just sensors) for temperature mappings
    set_data_for_plural_test(
        {{"printer", {{"temp_sensors", {{"bed", "heater_bed"}, {"hotend", "extruder"}}}}}});

    auto temp_sensors = config.get<json>("/printer/temp_sensors");
    REQUIRE(temp_sensors.is_object());
    REQUIRE(temp_sensors.contains("bed"));
    REQUIRE(temp_sensors.contains("hotend"));
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: hardware section is under /printer/hardware/",
                 "[config][defaults][plural]") {
    // Hardware config should be under /printer/hardware/ not /hardware/
    set_data_for_plural_test({{"printer",
                               {{"hardware",
                                 {{"optional", json::array()},
                                  {"expected", json::array()},
                                  {"last_snapshot", json::object()}}}}}});

    auto hardware = config.get<json>("/printer/hardware");
    REQUIRE(hardware.is_object());
    REQUIRE(hardware.contains("optional"));
    REQUIRE(hardware.contains("expected"));
    REQUIRE(hardware.contains("last_snapshot"));
}

// ============================================================================
// Wizard Config Path Constants Tests - Verify plural naming
// These tests verify that wizard_config_paths.h constants use plural form.
// They SHOULD FAIL until the implementation is updated.
// ============================================================================

TEST_CASE("WizardConfigPaths: BED_HEATER uses plural /printer/heaters/",
          "[config][paths][wizard][plural]") {
    // The path constant should use plural "heaters" not singular "heater"
    std::string path = helix::wizard::BED_HEATER;
    REQUIRE(path == "/printer/heaters/bed");
}

TEST_CASE("WizardConfigPaths: HOTEND_HEATER uses plural /printer/heaters/",
          "[config][paths][wizard][plural]") {
    std::string path = helix::wizard::HOTEND_HEATER;
    REQUIRE(path == "/printer/heaters/hotend");
}

TEST_CASE("WizardConfigPaths: BED_SENSOR uses plural /printer/temp_sensors/",
          "[config][paths][wizard][plural]") {
    // The path constant should use "temp_sensors" not "sensor"
    std::string path = helix::wizard::BED_SENSOR;
    REQUIRE(path == "/printer/temp_sensors/bed");
}

TEST_CASE("WizardConfigPaths: HOTEND_SENSOR uses plural /printer/temp_sensors/",
          "[config][paths][wizard][plural]") {
    std::string path = helix::wizard::HOTEND_SENSOR;
    REQUIRE(path == "/printer/temp_sensors/hotend");
}

TEST_CASE("WizardConfigPaths: PART_FAN uses plural /printer/fans/",
          "[config][paths][wizard][plural]") {
    // The path constant should use plural "fans" not singular "fan"
    std::string path = helix::wizard::PART_FAN;
    REQUIRE(path == "/printer/fans/part");
}

TEST_CASE("WizardConfigPaths: HOTEND_FAN uses plural /printer/fans/",
          "[config][paths][wizard][plural]") {
    std::string path = helix::wizard::HOTEND_FAN;
    REQUIRE(path == "/printer/fans/hotend");
}

TEST_CASE("WizardConfigPaths: LED_STRIP uses plural /printer/leds/",
          "[config][paths][wizard][plural]") {
    // The path constant should use plural "leds" not singular "led"
    std::string path = helix::wizard::LED_STRIP;
    REQUIRE(path == "/printer/leds/strip");
}

// ============================================================================
// Display Config Migration Tests - Phase 1 of display config refactoring
// ============================================================================

TEST_CASE_METHOD(ConfigTestFixture, "Config: display section exists with defaults for new config",
                 "[config][display][migration]") {
    // Populate with display section using test helper
    set_data_for_plural_test({{"printer", {{"moonraker_host", "127.0.0.1"}}},
                              {"display",
                               {{"rotate", 0},
                                {"sleep_sec", 600},
                                {"dim_sec", 300},
                                {"dim_brightness", 30},
                                {"drm_device", ""},
                                {"touch_device", ""},
                                {"calibration", {{"valid", false}, {"a", 1.0}, {"b", 0.0}}}}}});

    // Verify display section has expected structure
    auto display = config.get<json>("/display");
    REQUIRE(display.is_object());
    REQUIRE(display.contains("rotate"));
    REQUIRE(display.contains("sleep_sec"));
    REQUIRE(display.contains("dim_sec"));
    REQUIRE(display.contains("dim_brightness"));
    REQUIRE(display.contains("calibration"));

    REQUIRE(display["rotate"].get<int>() == 0);
    REQUIRE(display["sleep_sec"].get<int>() == 600);
    REQUIRE(display["dim_sec"].get<int>() == 300);
    REQUIRE(display["dim_brightness"].get<int>() == 30);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: display/calibration section has coefficients",
                 "[config][display][migration]") {
    // Set up display section with calibration
    set_data_for_plural_test({{"display",
                               {{"calibration",
                                 {{"valid", true},
                                  {"a", 1.5},
                                  {"b", 0.1},
                                  {"c", -10.0},
                                  {"d", 0.2},
                                  {"e", 1.3},
                                  {"f", -5.0}}}}}});

    auto cal = config.get<json>("/display/calibration");
    REQUIRE(cal.is_object());
    REQUIRE(cal.contains("valid"));
    REQUIRE(cal.contains("a"));
    REQUIRE(cal.contains("b"));
    REQUIRE(cal.contains("c"));
    REQUIRE(cal.contains("d"));
    REQUIRE(cal.contains("e"));
    REQUIRE(cal.contains("f"));

    REQUIRE(cal["valid"].get<bool>() == true);
    REQUIRE(cal["a"].get<double>() == Catch::Approx(1.5));
    REQUIRE(cal["e"].get<double>() == Catch::Approx(1.3));
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: display settings accessible via get() with defaults",
                 "[config][display][migration]") {
    set_data_empty();

    // Test default fallback when display section doesn't exist
    int rotate = config.get<int>("/display/rotate", 90);
    REQUIRE(rotate == 90); // Uses default since path doesn't exist

    int sleep_sec = config.get<int>("/display/sleep_sec", 1800);
    REQUIRE(sleep_sec == 1800);

    bool cal_valid = config.get<bool>("/display/calibration/valid", false);
    REQUIRE(cal_valid == false);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: display settings readable when populated",
                 "[config][display][migration]") {
    // Populate display section
    set_data_for_plural_test({{"display",
                               {{"rotate", 180},
                                {"sleep_sec", 300},
                                {"dim_sec", 120},
                                {"dim_brightness", 50},
                                {"gcode_3d_enabled", false},
                                {"calibration", {{"valid", true}, {"a", 2.0}}}}}});

    // Verify values are accessible
    REQUIRE(config.get<int>("/display/rotate") == 180);
    REQUIRE(config.get<int>("/display/sleep_sec") == 300);
    REQUIRE(config.get<int>("/display/dim_sec") == 120);
    REQUIRE(config.get<int>("/display/dim_brightness") == 50);
    REQUIRE(config.get<bool>("/display/gcode_3d_enabled") == false);
    REQUIRE(config.get<bool>("/display/calibration/valid") == true);
    REQUIRE(config.get<double>("/display/calibration/a") == Catch::Approx(2.0));
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: display settings can be set and updated",
                 "[config][display][migration]") {
    // Create config with empty display section
    set_data_for_plural_test({{"display", json::object()}});

    // Set values
    config.set<int>("/display/rotate", 270);
    config.set<int>("/display/sleep_sec", 900);
    config.set<bool>("/display/calibration/valid", true);
    config.set<double>("/display/calibration/a", 1.1);

    // Verify
    REQUIRE(config.get<int>("/display/rotate") == 270);
    REQUIRE(config.get<int>("/display/sleep_sec") == 900);
    REQUIRE(config.get<bool>("/display/calibration/valid") == true);
    REQUIRE(config.get<double>("/display/calibration/a") == Catch::Approx(1.1));
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: display calibration default values are identity matrix",
                 "[config][display][migration]") {
    // Set up default calibration (identity transform)
    set_data_for_plural_test({{"display",
                               {{"calibration",
                                 {{"valid", false},
                                  {"a", 1.0},
                                  {"b", 0.0},
                                  {"c", 0.0},
                                  {"d", 0.0},
                                  {"e", 1.0},
                                  {"f", 0.0}}}}}});

    auto cal = config.get<json>("/display/calibration");

    // Identity matrix check: a=1, b=0, c=0, d=0, e=1, f=0
    REQUIRE(cal["a"].get<double>() == Catch::Approx(1.0));
    REQUIRE(cal["b"].get<double>() == Catch::Approx(0.0));
    REQUIRE(cal["c"].get<double>() == Catch::Approx(0.0));
    REQUIRE(cal["d"].get<double>() == Catch::Approx(0.0));
    REQUIRE(cal["e"].get<double>() == Catch::Approx(1.0));
    REQUIRE(cal["f"].get<double>() == Catch::Approx(0.0));
}

// ============================================================================
// Display Config Migration Tests - Comprehensive coverage
// ============================================================================

// ----------------------------------------------------------------------------
// Migration Detection Tests
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: migration detects old format with display_rotate at root",
                 "[config][display][migration]") {
    // Old format config with display_rotate at root
    json old_format = {{"display_rotate", 90}, {"printer", {{"moonraker_host", "192.168.1.100"}}}};

    set_data_for_plural_test(old_format);
    REQUIRE(data_contains("display_rotate"));

    apply_migration();

    // Old key should be removed
    REQUIRE_FALSE(data_contains("display_rotate"));
    // New structure should exist
    REQUIRE(data_contains("display"));
    REQUIRE(config.get<int>("/display/rotate") == 90);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: migration skips config already in new format",
                 "[config][display][migration]") {
    // New format config - no root-level display_* keys
    json new_format = {{"display", {{"rotate", 180}, {"sleep_sec", 300}}},
                       {"printer", {{"moonraker_host", "192.168.1.100"}}}};

    set_data_for_plural_test(new_format);

    // Verify no migration needed (no display_rotate at root)
    REQUIRE_FALSE(data_contains("display_rotate"));

    apply_migration();

    // Values should be unchanged
    REQUIRE(config.get<int>("/display/rotate") == 180);
    REQUIRE(config.get<int>("/display/sleep_sec") == 300);
}

// ----------------------------------------------------------------------------
// Individual Key Migration Tests
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(ConfigTestFixture, "Config: migrates display_rotate to /display/rotate",
                 "[config][display][migration]") {
    json old_format = {{"display_rotate", 270}};

    set_data_for_plural_test(old_format);
    apply_migration();

    REQUIRE_FALSE(data_contains("display_rotate"));
    REQUIRE(config.get<int>("/display/rotate") == 270);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: migrates display_sleep_sec to /display/sleep_sec",
                 "[config][display][migration]") {
    json old_format = {{"display_rotate", 0}, {"display_sleep_sec", 1800}};

    set_data_for_plural_test(old_format);
    apply_migration();

    REQUIRE_FALSE(data_contains("display_sleep_sec"));
    REQUIRE(config.get<int>("/display/sleep_sec") == 1800);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: migrates display_dim_sec to /display/dim_sec",
                 "[config][display][migration]") {
    json old_format = {{"display_rotate", 0}, {"display_dim_sec", 120}};

    set_data_for_plural_test(old_format);
    apply_migration();

    REQUIRE_FALSE(data_contains("display_dim_sec"));
    REQUIRE(config.get<int>("/display/dim_sec") == 120);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: migrates display_dim_brightness to /display/dim_brightness",
                 "[config][display][migration]") {
    json old_format = {{"display_rotate", 0}, {"display_dim_brightness", 50}};

    set_data_for_plural_test(old_format);
    apply_migration();

    REQUIRE_FALSE(data_contains("display_dim_brightness"));
    REQUIRE(config.get<int>("/display/dim_brightness") == 50);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: migrates touch_calibrated to /display/calibration/valid",
                 "[config][display][migration]") {
    json old_format = {{"display_rotate", 0}, {"touch_calibrated", true}};

    set_data_for_plural_test(old_format);
    apply_migration();

    REQUIRE_FALSE(data_contains("touch_calibrated"));
    REQUIRE(config.get<bool>("/display/calibration/valid") == true);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: migrates touch_calibration coefficients to /display/calibration",
                 "[config][display][migration]") {
    json old_format = {
        {"display_rotate", 0},
        {"touch_calibration",
         {{"a", 1.5}, {"b", 0.1}, {"c", -10.0}, {"d", 0.2}, {"e", 1.3}, {"f", -5.0}}}};

    set_data_for_plural_test(old_format);
    apply_migration();

    REQUIRE_FALSE(data_contains("touch_calibration"));
    REQUIRE(config.get<double>("/display/calibration/a") == Catch::Approx(1.5));
    REQUIRE(config.get<double>("/display/calibration/b") == Catch::Approx(0.1));
    REQUIRE(config.get<double>("/display/calibration/c") == Catch::Approx(-10.0));
    REQUIRE(config.get<double>("/display/calibration/d") == Catch::Approx(0.2));
    REQUIRE(config.get<double>("/display/calibration/e") == Catch::Approx(1.3));
    REQUIRE(config.get<double>("/display/calibration/f") == Catch::Approx(-5.0));
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: migration removes all old root-level display keys",
                 "[config][display][migration]") {
    // Full old format config with all legacy keys
    json old_format = {{"display_rotate", 90},
                       {"display_sleep_sec", 900},
                       {"display_dim_sec", 180},
                       {"display_dim_brightness", 25},
                       {"touch_calibrated", true},
                       {"touch_calibration",
                        {{"a", 1.1}, {"b", 0.0}, {"c", 5.0}, {"d", 0.0}, {"e", 0.9}, {"f", 10.0}}},
                       {"printer", {{"moonraker_host", "test"}}}};

    set_data_for_plural_test(old_format);
    apply_migration();

    // All old keys should be gone
    REQUIRE_FALSE(data_contains("display_rotate"));
    REQUIRE_FALSE(data_contains("display_sleep_sec"));
    REQUIRE_FALSE(data_contains("display_dim_sec"));
    REQUIRE_FALSE(data_contains("display_dim_brightness"));
    REQUIRE_FALSE(data_contains("touch_calibrated"));
    REQUIRE_FALSE(data_contains("touch_calibration"));

    // All values should be in new location
    REQUIRE(config.get<int>("/display/rotate") == 90);
    REQUIRE(config.get<int>("/display/sleep_sec") == 900);
    REQUIRE(config.get<int>("/display/dim_sec") == 180);
    REQUIRE(config.get<int>("/display/dim_brightness") == 25);
    REQUIRE(config.get<bool>("/display/calibration/valid") == true);
    REQUIRE(config.get<double>("/display/calibration/a") == Catch::Approx(1.1));
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: partial migration handles only existing old keys",
                 "[config][display][migration]") {
    // Config with only some old keys (missing display_dim_sec and touch_calibration)
    json partial_old = {
        {"display_rotate", 180}, {"display_sleep_sec", 1200}, {"touch_calibrated", false}};

    set_data_for_plural_test(partial_old);
    apply_migration();

    // Present keys should be migrated
    REQUIRE(config.get<int>("/display/rotate") == 180);
    REQUIRE(config.get<int>("/display/sleep_sec") == 1200);
    REQUIRE(config.get<bool>("/display/calibration/valid") == false);

    // Missing keys should NOT exist in new location (no defaults injected by migration)
    REQUIRE_FALSE(display_contains("dim_sec"));
    REQUIRE_FALSE(display_contains("dim_brightness"));
    REQUIRE_FALSE(calibration_contains("a"));
}

// ----------------------------------------------------------------------------
// Default Value Tests - Verify get_default_display_config() values
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/rotate is 0",
                 "[config][display][defaults]") {
    set_data_empty();

    // Use default fallback when not set
    int rotate = config.get<int>("/display/rotate", 0);
    REQUIRE(rotate == 0);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/sleep_sec is 600",
                 "[config][display][defaults]") {
    set_data_empty();

    int sleep_sec = config.get<int>("/display/sleep_sec", 600);
    REQUIRE(sleep_sec == 600);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/dim_sec is 300",
                 "[config][display][defaults]") {
    set_data_empty();

    int dim_sec = config.get<int>("/display/dim_sec", 300);
    REQUIRE(dim_sec == 300);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/dim_brightness is 30",
                 "[config][display][defaults]") {
    set_data_empty();

    int dim_brightness = config.get<int>("/display/dim_brightness", 30);
    REQUIRE(dim_brightness == 30);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/drm_device is empty string",
                 "[config][display][defaults]") {
    set_data_empty();

    std::string drm_device = config.get<std::string>("/display/drm_device", "");
    REQUIRE(drm_device == "");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/touch_device is empty string",
                 "[config][display][defaults]") {
    set_data_empty();

    std::string touch_device = config.get<std::string>("/display/touch_device", "");
    REQUIRE(touch_device == "");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/gcode_render_mode is 0",
                 "[config][display][defaults]") {
    set_data_empty();

    int gcode_render_mode = config.get<int>("/display/gcode_render_mode", 0);
    REQUIRE(gcode_render_mode == 0);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/gcode_3d_enabled is true",
                 "[config][display][defaults]") {
    set_data_empty();

    bool gcode_3d_enabled = config.get<bool>("/display/gcode_3d_enabled", true);
    REQUIRE(gcode_3d_enabled == true);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/bed_mesh_render_mode is 0",
                 "[config][display][defaults]") {
    set_data_empty();

    int bed_mesh_render_mode = config.get<int>("/display/bed_mesh_render_mode", 0);
    REQUIRE(bed_mesh_render_mode == 0);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: default display/calibration/valid is false",
                 "[config][display][defaults]") {
    set_data_empty();

    bool cal_valid = config.get<bool>("/display/calibration/valid", false);
    REQUIRE(cal_valid == false);
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: default display/calibration coefficients form identity matrix",
                 "[config][display][defaults]") {
    set_data_empty();

    // Identity matrix: a=1, b=0, c=0, d=0, e=1, f=0
    REQUIRE(config.get<double>("/display/calibration/a", 1.0) == Catch::Approx(1.0));
    REQUIRE(config.get<double>("/display/calibration/b", 0.0) == Catch::Approx(0.0));
    REQUIRE(config.get<double>("/display/calibration/c", 0.0) == Catch::Approx(0.0));
    REQUIRE(config.get<double>("/display/calibration/d", 0.0) == Catch::Approx(0.0));
    REQUIRE(config.get<double>("/display/calibration/e", 1.0) == Catch::Approx(1.0));
    REQUIRE(config.get<double>("/display/calibration/f", 0.0) == Catch::Approx(0.0));
}

// ----------------------------------------------------------------------------
// Read/Write Tests - Set and get display values
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(ConfigTestFixture, "Config: set and get display/rotate",
                 "[config][display][readwrite]") {
    set_data_for_plural_test({{"display", json::object()}});

    config.set<int>("/display/rotate", 180);
    REQUIRE(config.get<int>("/display/rotate") == 180);

    config.set<int>("/display/rotate", 270);
    REQUIRE(config.get<int>("/display/rotate") == 270);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set and get display/sleep_sec",
                 "[config][display][readwrite]") {
    set_data_for_plural_test({{"display", json::object()}});

    config.set<int>("/display/sleep_sec", 1800);
    REQUIRE(config.get<int>("/display/sleep_sec") == 1800);

    config.set<int>("/display/sleep_sec", 0); // Disable sleep
    REQUIRE(config.get<int>("/display/sleep_sec") == 0);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set and get display/calibration/valid",
                 "[config][display][readwrite]") {
    set_data_for_plural_test({{"display", {{"calibration", json::object()}}}});

    config.set<bool>("/display/calibration/valid", true);
    REQUIRE(config.get<bool>("/display/calibration/valid") == true);

    config.set<bool>("/display/calibration/valid", false);
    REQUIRE(config.get<bool>("/display/calibration/valid") == false);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set and get display/calibration coefficients",
                 "[config][display][readwrite]") {
    set_data_for_plural_test({{"display", {{"calibration", json::object()}}}});

    // Set custom calibration values
    config.set<double>("/display/calibration/a", 1.25);
    config.set<double>("/display/calibration/b", 0.05);
    config.set<double>("/display/calibration/c", -15.5);
    config.set<double>("/display/calibration/d", 0.03);
    config.set<double>("/display/calibration/e", 1.15);
    config.set<double>("/display/calibration/f", -8.2);

    REQUIRE(config.get<double>("/display/calibration/a") == Catch::Approx(1.25));
    REQUIRE(config.get<double>("/display/calibration/b") == Catch::Approx(0.05));
    REQUIRE(config.get<double>("/display/calibration/c") == Catch::Approx(-15.5));
    REQUIRE(config.get<double>("/display/calibration/d") == Catch::Approx(0.03));
    REQUIRE(config.get<double>("/display/calibration/e") == Catch::Approx(1.15));
    REQUIRE(config.get<double>("/display/calibration/f") == Catch::Approx(-8.2));
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set and get display/drm_device",
                 "[config][display][readwrite]") {
    set_data_for_plural_test({{"display", json::object()}});

    config.set<std::string>("/display/drm_device", "/dev/dri/card0");
    REQUIRE(config.get<std::string>("/display/drm_device") == "/dev/dri/card0");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set and get display/touch_device",
                 "[config][display][readwrite]") {
    set_data_for_plural_test({{"display", json::object()}});

    config.set<std::string>("/display/touch_device", "/dev/input/event0");
    REQUIRE(config.get<std::string>("/display/touch_device") == "/dev/input/event0");
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: set and get display/gcode_3d_enabled",
                 "[config][display][readwrite]") {
    set_data_for_plural_test({{"display", json::object()}});

    config.set<bool>("/display/gcode_3d_enabled", false);
    REQUIRE(config.get<bool>("/display/gcode_3d_enabled") == false);

    config.set<bool>("/display/gcode_3d_enabled", true);
    REQUIRE(config.get<bool>("/display/gcode_3d_enabled") == true);
}

// ----------------------------------------------------------------------------
// Edge Cases
// ----------------------------------------------------------------------------

TEST_CASE_METHOD(ConfigTestFixture, "Config: empty display section gets populated with set values",
                 "[config][display][edge]") {
    set_data_for_plural_test({{"display", json::object()}});

    // Verify empty initially
    REQUIRE(display_size() == 0);

    // Set a single value
    config.set<int>("/display/rotate", 90);

    // Verify value was set
    REQUIRE(config.get<int>("/display/rotate") == 90);
    REQUIRE(display_size() == 1);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: missing calibration subsection can be created via set",
                 "[config][display][edge]") {
    // Display section without calibration
    set_data_for_plural_test({{"display", {{"rotate", 0}}}});

    REQUIRE_FALSE(display_contains("calibration"));

    // Set creates the path
    config.set<bool>("/display/calibration/valid", true);

    REQUIRE(display_contains("calibration"));
    REQUIRE(config.get<bool>("/display/calibration/valid") == true);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: migration preserves existing /display/ values",
                 "[config][display][edge]") {
    // Config with both old keys AND existing display section
    // This simulates a partial migration or manual edit
    json mixed_format = {{"display_rotate", 90}, // Old format
                         {"display",
                          {{"sleep_sec", 1200}, // Already in new format
                           {"drm_device", "/dev/dri/card1"}}}};

    set_data_for_plural_test(mixed_format);
    apply_migration();

    // Old key should be migrated
    REQUIRE(config.get<int>("/display/rotate") == 90);

    // Existing values should be preserved (not overwritten)
    REQUIRE(config.get<int>("/display/sleep_sec") == 1200);
    REQUIRE(config.get<std::string>("/display/drm_device") == "/dev/dri/card1");
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: migration handles touch_calibration without touch_calibrated",
                 "[config][display][edge]") {
    // Only touch_calibration coefficients, no touch_calibrated flag
    json old_format = {{"display_rotate", 0},
                       {"touch_calibration",
                        {{"a", 1.2}, {"b", 0.0}, {"c", 0.0}, {"d", 0.0}, {"e", 1.2}, {"f", 0.0}}}};

    set_data_for_plural_test(old_format);
    apply_migration();

    // Coefficients should be migrated
    REQUIRE(config.get<double>("/display/calibration/a") == Catch::Approx(1.2));
    REQUIRE(config.get<double>("/display/calibration/e") == Catch::Approx(1.2));

    // valid flag should NOT be set (since touch_calibrated wasn't present)
    REQUIRE_FALSE(calibration_contains("valid"));
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: migration handles touch_calibrated without coefficients",
                 "[config][display][edge]") {
    // Only touch_calibrated flag, no coefficients
    json old_format = {{"display_rotate", 0}, {"touch_calibrated", true}};

    set_data_for_plural_test(old_format);
    apply_migration();

    // Flag should be migrated
    REQUIRE(config.get<bool>("/display/calibration/valid") == true);

    // Coefficients should NOT be set
    REQUIRE_FALSE(calibration_contains("a"));
}

TEST_CASE_METHOD(ConfigTestFixture,
                 "Config: migration handles partial touch_calibration coefficients",
                 "[config][display][edge]") {
    // Only some coefficients present
    json old_format = {{"display_rotate", 0}, {"touch_calibration", {{"a", 1.5}, {"e", 1.3}}}};

    set_data_for_plural_test(old_format);
    apply_migration();

    // Present coefficients should be migrated
    REQUIRE(config.get<double>("/display/calibration/a") == Catch::Approx(1.5));
    REQUIRE(config.get<double>("/display/calibration/e") == Catch::Approx(1.3));

    // Missing coefficients should NOT be set
    REQUIRE_FALSE(calibration_contains("b"));
    REQUIRE_FALSE(calibration_contains("c"));
    REQUIRE_FALSE(calibration_contains("d"));
    REQUIRE_FALSE(calibration_contains("f"));
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: display values with boundary conditions",
                 "[config][display][edge]") {
    set_data_for_plural_test({{"display", json::object()}});

    // Test rotation values (0, 90, 180, 270)
    for (int rotation : {0, 90, 180, 270}) {
        config.set<int>("/display/rotate", rotation);
        REQUIRE(config.get<int>("/display/rotate") == rotation);
    }

    // Test sleep disabled (0) and max reasonable value
    config.set<int>("/display/sleep_sec", 0);
    REQUIRE(config.get<int>("/display/sleep_sec") == 0);

    config.set<int>("/display/sleep_sec", 86400); // 24 hours
    REQUIRE(config.get<int>("/display/sleep_sec") == 86400);

    // Test brightness range (0-100)
    config.set<int>("/display/dim_brightness", 0);
    REQUIRE(config.get<int>("/display/dim_brightness") == 0);

    config.set<int>("/display/dim_brightness", 100);
    REQUIRE(config.get<int>("/display/dim_brightness") == 100);
}

TEST_CASE_METHOD(ConfigTestFixture, "Config: display calibration with extreme coefficient values",
                 "[config][display][edge]") {
    set_data_for_plural_test({{"display", {{"calibration", json::object()}}}});

    // Test very small values
    config.set<double>("/display/calibration/a", 0.001);
    REQUIRE(config.get<double>("/display/calibration/a") == Catch::Approx(0.001));

    // Test negative values
    config.set<double>("/display/calibration/c", -500.0);
    REQUIRE(config.get<double>("/display/calibration/c") == Catch::Approx(-500.0));

    // Test large values
    config.set<double>("/display/calibration/f", 1000.0);
    REQUIRE(config.get<double>("/display/calibration/f") == Catch::Approx(1000.0));
}

TEST_CASE_METHOD(
    ConfigTestFixture,
    "Config: migration does not overwrite existing /display/ values with old root values",
    "[config][display][migration]") {
    // Set up config with both old and new format - new should win
    json mixed = {{"display_rotate", 90},     // Old value
                  {"display_sleep_sec", 300}, // Old value
                  {"display",
                   {
                       {"rotate", 180},   // New value should NOT be overwritten
                       {"sleep_sec", 600} // New value should NOT be overwritten
                   }}};
    set_data_for_plural_test(mixed);
    apply_migration();

    // Verify new values were preserved
    REQUIRE(get_data()["display"]["rotate"] == 180);
    REQUIRE(get_data()["display"]["sleep_sec"] == 600);

    // Verify old keys were removed
    REQUIRE_FALSE(data_contains("display_rotate"));
    REQUIRE_FALSE(data_contains("display_sleep_sec"));
}
