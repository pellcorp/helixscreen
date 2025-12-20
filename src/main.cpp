// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 HelixScreen

/**
 * @file main.cpp
 * @brief Application entry point
 *
 * This file is intentionally minimal. All application logic is implemented
 * in the Application class (src/application/application.cpp).
 *
 * @see Application
 */

#include "application.h"

int main(int argc, char** argv) {
    Application app;
    return app.run(argc, argv);
}
