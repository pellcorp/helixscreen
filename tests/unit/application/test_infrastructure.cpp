// Copyright 2025 HelixScreen
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string>

#include "../../catch_amalgamated.hpp"

// Simple test to verify our new test directories work
TEST_CASE("New test directories are working", "[application][infrastructure]") {
    SECTION("Basic test execution") {
        REQUIRE(1 + 1 == 2);
    }

    SECTION("String operations") {
        std::string test = "refactoring";
        REQUIRE(test == "refactoring");
    }
}
