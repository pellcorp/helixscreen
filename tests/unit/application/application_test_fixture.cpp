// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application_test_fixture.h"

ApplicationTestFixture::ApplicationTestFixture() : LVGLTestFixture() {
    configure_test_mode();
}

ApplicationTestFixture::~ApplicationTestFixture() = default;

void ApplicationTestFixture::configure_test_mode() {
    m_config = RuntimeConfig{};
    m_config.test_mode = true;
    m_config.skip_splash = true;
    m_config.sim_speedup = 10.0; // Speed up tests
}

void ApplicationTestFixture::configure_real_moonraker() {
    configure_test_mode();
    m_config.use_real_moonraker = true;
}

void ApplicationTestFixture::set_sim_speedup(double speedup) {
    m_config.sim_speedup = speedup;
}

void ApplicationTestFixture::reset_mocks() {
    m_mock_state.reset();
    configure_test_mode();
}
