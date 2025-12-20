// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "../../lvgl_test_fixture.h"
#include "../../mocks/mock_printer_state.h"
#include "runtime_config.h"

#include <memory>

// Forward declarations
class MoonrakerClientMock;
class MoonrakerAPI;

/**
 * @brief Test fixture for application module tests
 *
 * Extends LVGLTestFixture with helpers specific to testing application
 * initialization and lifecycle. Provides:
 * - Pre-configured RuntimeConfig for test mode
 * - Mock Moonraker client/API factories
 * - Common test utilities for application modules
 *
 * Usage:
 * @code
 * TEST_CASE_METHOD(ApplicationTestFixture, "Test name", "[application]") {
 *     // RuntimeConfig is already set up for test mode
 *     REQUIRE(config().is_test_mode());
 *
 *     // Create test objects on test_screen()
 *     lv_obj_t* panel = lv_obj_create(test_screen());
 *     process_lvgl(50);
 * }
 * @endcode
 */
class ApplicationTestFixture : public LVGLTestFixture {
  public:
    ApplicationTestFixture();
    ~ApplicationTestFixture() override;

    // Non-copyable, non-movable
    ApplicationTestFixture(const ApplicationTestFixture&) = delete;
    ApplicationTestFixture& operator=(const ApplicationTestFixture&) = delete;
    ApplicationTestFixture(ApplicationTestFixture&&) = delete;
    ApplicationTestFixture& operator=(ApplicationTestFixture&&) = delete;

    /**
     * @brief Get test RuntimeConfig (test_mode=true by default)
     * @return Reference to the test configuration
     */
    RuntimeConfig& config() {
        return m_config;
    }
    const RuntimeConfig& config() const {
        return m_config;
    }

    /**
     * @brief Get shared mock printer state
     * @return Reference to mock state for coordinating mock behavior
     */
    MockPrinterState& mock_state() {
        return m_mock_state;
    }
    const MockPrinterState& mock_state() const {
        return m_mock_state;
    }

    /**
     * @brief Configure for test mode with mocks (default)
     *
     * Sets up RuntimeConfig with:
     * - test_mode = true
     * - All should_mock_*() return true
     */
    void configure_test_mode();

    /**
     * @brief Configure for test mode with real Moonraker
     *
     * Sets up RuntimeConfig with:
     * - test_mode = true
     * - use_real_moonraker = true
     */
    void configure_real_moonraker();

    /**
     * @brief Configure simulation speedup
     * @param speedup Factor to speed up simulation (1.0 = real-time)
     */
    void set_sim_speedup(double speedup);

    /**
     * @brief Reset all mock state to defaults
     *
     * Clears MockPrinterState and resets RuntimeConfig to test defaults.
     */
    void reset_mocks();

  protected:
    RuntimeConfig m_config;
    MockPrinterState m_mock_state;
};
