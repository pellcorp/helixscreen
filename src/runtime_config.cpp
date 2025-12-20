// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 HelixScreen

#include "runtime_config.h"

// Global runtime configuration instance
static RuntimeConfig g_runtime_config;

RuntimeConfig* get_runtime_config() {
    return &g_runtime_config;
}
