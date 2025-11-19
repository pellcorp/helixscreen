// Copyright (c) 2025 HelixScreen Project
// SPDX-License-Identifier: GPL-3.0-or-later
//
// TinyGL Test Runner - Main test execution program

#include "tinygl_test_framework.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <iomanip>
#include <iostream>

using namespace tinygl_test;

void print_separator(const std::string& title = "") {
    if (title.empty()) {
        std::cout << "═══════════════════════════════════════════════════════════════\n";
    } else {
        int padding = (60 - title.length()) / 2;
        std::cout << "═";
        for (int i = 0; i < padding; i++) std::cout << "═";
        std::cout << " " << title << " ";
        for (int i = 0; i < padding; i++) std::cout << "═";
        std::cout << "═\n";
    }
}

void print_metrics(const std::string& name, const ImageMetrics& metrics) {
    std::cout << "\n📊 " << name << " Image Quality Metrics:\n";
    std::cout << "  • MSE:          " << std::fixed << std::setprecision(2) << metrics.mse << "\n";
    std::cout << "  • PSNR:         " << std::fixed << std::setprecision(2) << metrics.psnr << " dB\n";
    std::cout << "  • SSIM:         " << std::fixed << std::setprecision(4) << metrics.ssim << "\n";
    std::cout << "  • Max Diff:     " << static_cast<int>(metrics.max_diff) << "/255\n";
    std::cout << "  • Diff Pixels:  " << metrics.diff_pixels << "\n";
}

void print_perf(const std::string& name, const PerfMetrics& metrics) {
    std::cout << "\n⚡ " << name << " Performance Metrics:\n";
    std::cout << "  • Frame Time:      " << std::fixed << std::setprecision(2)
              << metrics.frame_time_ms << " ms\n";
    std::cout << "  • FPS:             " << std::fixed << std::setprecision(1)
              << (1000.0 / metrics.frame_time_ms) << "\n";
    std::cout << "  • Vertices/sec:    " << std::scientific << std::setprecision(2)
              << metrics.vertices_per_second << "\n";
    std::cout << "  • Triangles/sec:   " << std::scientific << std::setprecision(2)
              << metrics.triangles_per_second << "\n";
    std::cout << "  • MPixels/sec:     " << std::fixed << std::setprecision(2)
              << (metrics.pixels_per_second / 1000000.0) << "\n";
}

void test_basic_rendering(TinyGLTestFramework& framework) {
    print_separator("Basic Rendering Test");

    // Test configuration
    SceneConfig config;
    config.width = 800;
    config.height = 600;
    config.enable_lighting = true;
    config.enable_smooth_shading = true;

    // Test 1: Sphere with varying tessellation
    std::cout << "\n🔵 Testing sphere tessellation levels...\n";

    for (int subdiv = 0; subdiv <= 3; subdiv++) {
        SphereTesselationScene sphere(subdiv);
        framework.render_scene(&sphere, config);

        std::string filename = "tests/tinygl/output/sphere_subdiv_" +
                              std::to_string(subdiv) + ".ppm";
        framework.save_screenshot(filename);

        // Benchmark
        auto perf = framework.benchmark_scene(&sphere, config, 100);
        std::cout << "  Subdivision " << subdiv
                  << ": " << sphere.get_triangle_count() << " triangles, "
                  << std::fixed << std::setprecision(2) << perf.frame_time_ms << " ms/frame\n";
    }
}

void test_gouraud_artifacts(TinyGLTestFramework& framework) {
    print_separator("Gouraud Shading Artifacts Test");

    SceneConfig config;
    config.enable_smooth_shading = true;

    GouraudArtifactScene scene;
    framework.render_scene(&scene, config);
    framework.save_screenshot("tests/tinygl/output/gouraud_artifacts.ppm");

    std::cout << "\n🎨 Gouraud artifact test rendered.\n";
    std::cout << "  Low-tessellation cylinder should show clear faceting.\n";
    std::cout << "  High-tessellation cylinder should appear smoother.\n";
}

void test_color_banding(TinyGLTestFramework& framework) {
    print_separator("Color Banding Test");

    SceneConfig config;

    ColorBandingScene scene;
    framework.render_scene(&scene, config);
    framework.save_screenshot("tests/tinygl/output/color_banding.ppm");

    std::cout << "\n🌈 Color banding test rendered.\n";
    std::cout << "  Gradient should show visible 8-bit quantization bands.\n";
    std::cout << "  Sphere lighting should show subtle banding in shadows.\n";
}

void test_performance_scaling(TinyGLTestFramework& framework) {
    print_separator("Performance Scaling Test");

    SceneConfig config;

    std::cout << "\n📈 Testing performance with increasing complexity...\n\n";

    // Test cube grids of increasing size
    for (int size = 2; size <= 8; size += 2) {
        CubeGridScene scene(size);
        auto perf = framework.benchmark_scene(&scene, config, 50);

        std::cout << "  Grid " << size << "×" << size << "×" << size
                  << " (" << scene.get_triangle_count() << " triangles): "
                  << std::fixed << std::setprecision(2) << perf.frame_time_ms << " ms, "
                  << std::setprecision(1) << (1000.0 / perf.frame_time_ms) << " FPS\n";
    }
}

void test_lighting_configurations(TinyGLTestFramework& framework) {
    print_separator("Lighting Configuration Test");

    SphereTesselationScene sphere(3);

    // Test different lighting setups
    std::vector<std::pair<std::string, SceneConfig>> configs = {
        {"no_lighting", {800, 600, true, false}},
        {"flat_shading", {800, 600, true, true, false, false}},
        {"gouraud_1_light", {800, 600, true, true, false, true, 1}},
        {"gouraud_2_lights", {800, 600, true, true, false, true, 2}},
        {"high_specular", {800, 600, true, true, false, true, 2, 0.3f, 0.5f, 128.0f}},
    };

    std::cout << "\n💡 Testing lighting configurations...\n\n";

    for (const auto& [name, config] : configs) {
        framework.render_scene(&sphere, config);
        std::string filename = "tests/tinygl/output/lighting_" + name + ".ppm";
        framework.save_screenshot(filename);

        auto perf = framework.benchmark_scene(&sphere, config, 50);
        std::cout << "  " << std::setw(20) << std::left << name
                  << ": " << std::fixed << std::setprecision(2)
                  << perf.frame_time_ms << " ms/frame\n";
    }
}

void generate_reference_images(TinyGLTestFramework& framework) {
    print_separator("Generating Reference Images");

    SceneConfig config;
    config.enable_lighting = true;
    config.enable_smooth_shading = true;
    config.ambient_intensity = 0.3f;
    config.specular_intensity = 0.05f;

    // Generate reference images for all test scenes
    std::vector<std::unique_ptr<TestScene>> scenes;
    scenes.push_back(std::make_unique<SphereTesselationScene>(3));
    scenes.push_back(std::make_unique<CubeGridScene>(4));
    scenes.push_back(std::make_unique<GouraudArtifactScene>());
    scenes.push_back(std::make_unique<ColorBandingScene>());

    std::cout << "\n📸 Generating reference images...\n";

    for (auto& scene : scenes) {
        framework.render_scene(scene.get(), config);
        std::string filename = "tests/tinygl/reference/" + scene->get_name() + ".ppm";
        // Replace spaces with underscores
        std::replace(filename.begin(), filename.end(), ' ', '_');
        framework.save_screenshot(filename);
        std::cout << "  ✓ " << scene->get_name() << "\n";
    }
}

int main(int argc, char** argv) {
    // Set up logging
    spdlog::set_level(spdlog::level::info);
    spdlog::set_pattern("[%H:%M:%S] %v");

    print_separator("TinyGL Test Framework");
    std::cout << "\n";
    std::cout << "  Testing TinyGL rendering quality and performance\n";
    std::cout << "  Output directory: tests/tinygl/output/\n";
    std::cout << "\n";

    // Create output directories
    std::filesystem::create_directories("tests/tinygl/output");
    std::filesystem::create_directories("tests/tinygl/reference");

    // Initialize test framework
    TinyGLTestFramework framework(800, 600);
    if (!framework.initialize()) {
        spdlog::error("Failed to initialize TinyGL test framework");
        return 1;
    }

    // Run test suites
    if (argc > 1) {
        std::string test_name = argv[1];

        if (test_name == "basic") {
            test_basic_rendering(framework);
        } else if (test_name == "gouraud") {
            test_gouraud_artifacts(framework);
        } else if (test_name == "banding") {
            test_color_banding(framework);
        } else if (test_name == "performance") {
            test_performance_scaling(framework);
        } else if (test_name == "lighting") {
            test_lighting_configurations(framework);
        } else if (test_name == "reference") {
            generate_reference_images(framework);
        } else {
            std::cout << "Unknown test: " << test_name << "\n";
            std::cout << "Available tests: basic, gouraud, banding, performance, lighting, reference\n";
            return 1;
        }
    } else {
        // Run all tests
        test_basic_rendering(framework);
        test_gouraud_artifacts(framework);
        test_color_banding(framework);
        test_lighting_configurations(framework);
        test_performance_scaling(framework);
        generate_reference_images(framework);
    }

    print_separator();
    std::cout << "\n✅ All tests completed!\n";
    std::cout << "\nView results:\n";
    std::cout << "  • macOS: open tests/tinygl/output/*.ppm\n";
    std::cout << "  • Linux: xdg-open tests/tinygl/output/*.ppm\n";
    std::cout << "\n";

    return 0;
}