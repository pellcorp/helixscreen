// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#include "capability_overrides.h"

#include "config.h"

#include <algorithm>
#include <spdlog/spdlog.h>

void CapabilityOverrides::load_from_config() {
    Config* cfg = Config::get_instance();
    if (!cfg) {
        spdlog::warn("[CapabilityOverrides] Config not available, using defaults");
        return;
    }

    // Get printer-specific path prefix
    std::string prefix = cfg->df();

    // Try to read capability_overrides from config
    // Path: /printers/<printer>/capability_overrides/<capability>
    auto read_override = [&](const std::string& name) {
        // Ensure path starts with '/' for valid JSON pointer
        std::string path = (prefix.empty() || prefix[0] != '/')
                               ? "/" + prefix + "capability_overrides/" + name
                               : prefix + "capability_overrides/" + name;
        std::string value = cfg->get<std::string>(path, "auto");
        overrides_[name] = parse_state(value);
    };

    read_override(capability::BED_LEVELING);
    read_override(capability::QGL);
    read_override(capability::Z_TILT);
    read_override(capability::NOZZLE_CLEAN);
    read_override(capability::HEAT_SOAK);
    read_override(capability::CHAMBER);

    spdlog::debug("[CapabilityOverrides] Loaded: {}", summary());
}

void CapabilityOverrides::set_printer_capabilities(const PrinterCapabilities& caps) {
    capabilities_ = caps;
    capabilities_set_ = true;
}

OverrideState CapabilityOverrides::get_override(const std::string& name) const {
    auto it = overrides_.find(name);
    if (it != overrides_.end()) {
        return it->second;
    }
    return OverrideState::AUTO;
}

void CapabilityOverrides::set_override(const std::string& name, OverrideState state) {
    overrides_[name] = state;
}

bool CapabilityOverrides::is_available(const std::string& name) const {
    OverrideState state = get_override(name);

    switch (state) {
    case OverrideState::ENABLE:
        return true;
    case OverrideState::DISABLE:
        return false;
    case OverrideState::AUTO:
    default:
        return get_auto_value(name);
    }
}

bool CapabilityOverrides::get_auto_value(const std::string& name) const {
    if (!capabilities_set_) {
        // No capabilities set, default to false for safety
        return false;
    }

    if (name == capability::BED_LEVELING) {
        return capabilities_.has_bed_mesh();
    } else if (name == capability::QGL) {
        return capabilities_.has_qgl();
    } else if (name == capability::Z_TILT) {
        return capabilities_.has_z_tilt();
    } else if (name == capability::NOZZLE_CLEAN) {
        return capabilities_.has_nozzle_clean_macro();
    } else if (name == capability::HEAT_SOAK) {
        return capabilities_.has_heat_soak_macro();
    } else if (name == capability::CHAMBER) {
        return capabilities_.supports_chamber();
    }

    // Unknown capability, default to false
    spdlog::warn("[CapabilityOverrides] Unknown capability: {}", name);
    return false;
}

bool CapabilityOverrides::save_to_config() {
    Config* cfg = Config::get_instance();
    if (!cfg) {
        spdlog::error("[CapabilityOverrides] Cannot save - Config not available");
        return false;
    }

    std::string prefix = cfg->df();

    for (const auto& [name, state] : overrides_) {
        // Ensure path starts with '/' for valid JSON pointer
        std::string path = (prefix.empty() || prefix[0] != '/')
                               ? "/" + prefix + "capability_overrides/" + name
                               : prefix + "capability_overrides/" + name;
        cfg->set<std::string>(path, state_to_string(state));
    }

    return cfg->save();
}

std::string CapabilityOverrides::summary() const {
    std::string result;

    auto append = [&](const std::string& name) {
        OverrideState state = get_override(name);
        bool effective = is_available(name);

        if (!result.empty())
            result += ", ";
        result += name + "=";

        switch (state) {
        case OverrideState::ENABLE:
            result += "ENABLE";
            break;
        case OverrideState::DISABLE:
            result += "DISABLE";
            break;
        case OverrideState::AUTO:
            result += effective ? "auto(Y)" : "auto(N)";
            break;
        }
    };

    append(capability::BED_LEVELING);
    append(capability::QGL);
    append(capability::Z_TILT);
    append(capability::NOZZLE_CLEAN);
    append(capability::HEAT_SOAK);
    append(capability::CHAMBER);

    return result;
}

OverrideState CapabilityOverrides::parse_state(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "enable" || lower == "enabled" || lower == "on" || lower == "true" ||
        lower == "yes" || lower == "1") {
        return OverrideState::ENABLE;
    } else if (lower == "disable" || lower == "disabled" || lower == "off" || lower == "false" ||
               lower == "no" || lower == "0") {
        return OverrideState::DISABLE;
    }

    // Default to AUTO for "auto" or any unrecognized value
    return OverrideState::AUTO;
}

std::string CapabilityOverrides::state_to_string(OverrideState state) {
    switch (state) {
    case OverrideState::ENABLE:
        return "enable";
    case OverrideState::DISABLE:
        return "disable";
    case OverrideState::AUTO:
    default:
        return "auto";
    }
}
