// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wifi_backend.h"

#include "runtime_config.h"
#include "spdlog/spdlog.h"
#include "wifi_backend_mock.h"

#ifdef __APPLE__
#include "wifi_backend_macos.h"
#else
#include "wifi_backend_wpa_supplicant.h"
#endif

std::unique_ptr<WifiBackend> WifiBackend::create(bool silent) {
    // In test mode, always use mock unless --real-wifi was specified
    if (get_runtime_config()->should_mock_wifi()) {
        spdlog::info("[WifiBackend] Test mode: using mock backend");
        auto mock = std::make_unique<WifiBackendMock>();
        mock->set_silent(silent);
        mock->start();
        return mock;
    }

#ifdef __APPLE__
    // macOS: Try CoreWLAN backend
    spdlog::debug("[WifiBackend] Attempting CoreWLAN backend for macOS");
    auto backend = std::make_unique<WifiBackendMacOS>();
    backend->set_silent(silent);
    WiFiError start_result = backend->start();

    if (start_result.success()) {
        spdlog::info("[WifiBackend] CoreWLAN backend started successfully");
        return backend;
    }

    // In production mode, don't fallback to mock - WiFi is simply unavailable
    spdlog::warn("[WifiBackend] CoreWLAN backend failed: {} - WiFi unavailable",
                 start_result.technical_msg);
    return nullptr;
#else
    // Linux: Try wpa_supplicant backend
    spdlog::debug("[WifiBackend] Attempting wpa_supplicant backend for Linux{}",
                  silent ? " (silent mode)" : "");
    auto backend = std::make_unique<WifiBackendWpaSupplicant>();
    backend->set_silent(silent);
    WiFiError start_result = backend->start();

    if (start_result.success()) {
        spdlog::info("[WifiBackend] wpa_supplicant backend started successfully");
        return backend;
    }

    // In production mode, don't fallback to mock - WiFi is simply unavailable
    spdlog::warn("[WifiBackend] wpa_supplicant backend failed: {} - WiFi unavailable",
                 start_result.technical_msg);
    return nullptr;
#endif
}