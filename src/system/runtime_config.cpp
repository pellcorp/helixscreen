// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 HelixScreen

#include "runtime_config.h"

#include "ams_state.h"

#include <cstdlib>

// Global runtime configuration instance
static RuntimeConfig g_runtime_config;

RuntimeConfig* get_runtime_config() {
    return &g_runtime_config;
}

bool RuntimeConfig::should_show_runout_modal() const {
    // If explicitly forced via env var, always show
    if (std::getenv("HELIX_FORCE_RUNOUT_MODAL") != nullptr) {
        return true;
    }
    // If AMS/MMU present (mock or real), suppress modal (runout during swaps is normal)
    if (AmsState::instance().is_available()) {
        return false;
    }
    // Otherwise, show modal
    return true;
}
