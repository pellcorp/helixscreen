// Copyright (C) 2025-2026 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * @file test_cleanup_helpers.cpp
 * @brief Tests for safe_delete_obj() and safe_delete_timer() helpers
 *
 * These helpers eliminate the if-delete-null pattern repeated in panel destructors.
 */

#include "../lvgl_test_fixture.h"
#include "ui/ui_cleanup_helpers.h"

#include "../catch_amalgamated.hpp"

using namespace helix::ui;

// ============================================================================
// safe_delete_obj() tests
// ============================================================================

TEST_CASE_METHOD(LVGLTestFixture, "safe_delete_obj deletes valid object and nulls pointer",
                 "[cleanup_helpers]") {
    lv_obj_t* obj = lv_obj_create(test_screen());
    REQUIRE(obj != nullptr);

    safe_delete_obj(obj);

    REQUIRE(obj == nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "safe_delete_obj is safe with nullptr", "[cleanup_helpers]") {
    lv_obj_t* obj = nullptr;

    // Should not crash
    safe_delete_obj(obj);

    REQUIRE(obj == nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "safe_delete_obj can be called multiple times safely",
                 "[cleanup_helpers]") {
    lv_obj_t* obj = lv_obj_create(test_screen());

    safe_delete_obj(obj);
    REQUIRE(obj == nullptr);

    // Second call should be safe (no double-free)
    safe_delete_obj(obj);
    REQUIRE(obj == nullptr);
}

// ============================================================================
// safe_delete_timer() tests
// ============================================================================

static void dummy_timer_cb(lv_timer_t*) {
    // No-op callback for test timers
}

TEST_CASE_METHOD(LVGLTestFixture, "safe_delete_timer deletes valid timer and nulls pointer",
                 "[cleanup_helpers]") {
    lv_timer_t* timer = lv_timer_create(dummy_timer_cb, 1000, nullptr);
    REQUIRE(timer != nullptr);

    safe_delete_timer(timer);

    REQUIRE(timer == nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "safe_delete_timer is safe with nullptr", "[cleanup_helpers]") {
    lv_timer_t* timer = nullptr;

    // Should not crash
    safe_delete_timer(timer);

    REQUIRE(timer == nullptr);
}

TEST_CASE_METHOD(LVGLTestFixture, "safe_delete_timer can be called multiple times safely",
                 "[cleanup_helpers]") {
    lv_timer_t* timer = lv_timer_create(dummy_timer_cb, 1000, nullptr);

    safe_delete_timer(timer);
    REQUIRE(timer == nullptr);

    // Second call should be safe (no double-free)
    safe_delete_timer(timer);
    REQUIRE(timer == nullptr);
}
