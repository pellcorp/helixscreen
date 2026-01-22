// SPDX-License-Identifier: GPL-3.0-or-later

#include "../catch_amalgamated.hpp"
#include "filament_database.h"

using namespace filament;
using Catch::Approx;

// ============================================================================
// find_material tests
// ============================================================================

TEST_CASE("find_material - exact name lookup", "[filament][database]") {
    auto result = find_material("PLA");
    REQUIRE(result.has_value());
    CHECK(std::string_view(result->name) == "PLA");
    CHECK(result->nozzle_min == 190);
    CHECK(result->nozzle_max == 220);
    CHECK(result->bed_temp == 60);
}

TEST_CASE("find_material - case insensitive lowercase", "[filament][database]") {
    auto result = find_material("pla");
    REQUIRE(result.has_value());
    CHECK(std::string_view(result->name) == "PLA");
}

TEST_CASE("find_material - case insensitive mixed case", "[filament][database]") {
    auto result = find_material("Pla");
    REQUIRE(result.has_value());
    CHECK(std::string_view(result->name) == "PLA");
}

TEST_CASE("find_material - unknown material returns nullopt", "[filament][database]") {
    auto result = find_material("FooBar");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("find_material - empty string returns nullopt", "[filament][database]") {
    auto result = find_material("");
    CHECK_FALSE(result.has_value());
}

TEST_CASE("find_material - verifies all new fields populated", "[filament][database]") {
    auto result = find_material("ABS");
    REQUIRE(result.has_value());

    // Basic fields
    CHECK(std::string_view(result->name) == "ABS");
    CHECK(std::string_view(result->category) == "Engineering");

    // Temperature fields
    CHECK(result->nozzle_min > 0);
    CHECK(result->nozzle_max > result->nozzle_min);
    CHECK(result->bed_temp > 0);

    // Drying fields
    CHECK(result->dry_temp_c == 60);
    CHECK(result->dry_time_min == 240);

    // Physical properties
    CHECK(result->density_g_cm3 == Approx(1.04f).epsilon(0.01f));

    // Classification
    CHECK(result->chamber_temp_c == 50);
    CHECK(std::string_view(result->compat_group) == "ABS_ASA");
}

// ============================================================================
// resolve_alias tests
// ============================================================================

TEST_CASE("resolve_alias - Nylon resolves to PA", "[filament][database][alias]") {
    auto resolved = resolve_alias("Nylon");
    CHECK(resolved == "PA");
}

TEST_CASE("resolve_alias - ULTEM resolves to PEI", "[filament][database][alias]") {
    auto resolved = resolve_alias("ULTEM");
    CHECK(resolved == "PEI");
}

TEST_CASE("resolve_alias - case insensitive", "[filament][database][alias]") {
    auto resolved = resolve_alias("nylon");
    CHECK(resolved == "PA");
}

TEST_CASE("resolve_alias - non-alias returns original", "[filament][database][alias]") {
    auto resolved = resolve_alias("PLA");
    CHECK(resolved == "PLA");
}

TEST_CASE("resolve_alias - empty string returns empty", "[filament][database][alias]") {
    auto resolved = resolve_alias("");
    CHECK(resolved == "");
}

TEST_CASE("resolve_alias - Polycarbonate resolves to PC", "[filament][database][alias]") {
    auto resolved = resolve_alias("Polycarbonate");
    CHECK(resolved == "PC");
}

// ============================================================================
// find_material with aliases
// ============================================================================

TEST_CASE("find_material - Nylon alias returns PA info", "[filament][database][alias]") {
    auto result = find_material("Nylon");
    REQUIRE(result.has_value());
    CHECK(std::string_view(result->name) == "PA");
    CHECK(std::string_view(result->compat_group) == "PA");
}

TEST_CASE("find_material - Polycarbonate alias returns PC info", "[filament][database][alias]") {
    auto result = find_material("Polycarbonate");
    REQUIRE(result.has_value());
    CHECK(std::string_view(result->name) == "PC");
    CHECK(std::string_view(result->compat_group) == "PC");
}

TEST_CASE("find_material - ULTEM alias returns PEI info", "[filament][database][alias]") {
    auto result = find_material("ULTEM");
    REQUIRE(result.has_value());
    CHECK(std::string_view(result->name) == "PEI");
    CHECK(std::string_view(result->compat_group) == "HIGH_TEMP");
}

// ============================================================================
// are_materials_compatible tests
// ============================================================================

TEST_CASE("are_materials_compatible - same group PLA variants", "[filament][database][compat]") {
    CHECK(are_materials_compatible("PLA", "PLA-CF"));
    CHECK(are_materials_compatible("PLA", "PLA+"));
    CHECK(are_materials_compatible("Silk PLA", "Matte PLA"));
}

TEST_CASE("are_materials_compatible - same group ABS and ASA", "[filament][database][compat]") {
    CHECK(are_materials_compatible("ABS", "ASA"));
    CHECK(are_materials_compatible("ABS", "HIPS")); // HIPS is in ABS_ASA group
    CHECK(are_materials_compatible("PC-ABS", "ASA")); // PC-ABS blend is ABS_ASA group
}

TEST_CASE("are_materials_compatible - different groups incompatible", "[filament][database][compat]") {
    CHECK_FALSE(are_materials_compatible("PLA", "PETG"));
    CHECK_FALSE(are_materials_compatible("PLA", "ABS"));
    CHECK_FALSE(are_materials_compatible("PETG", "ABS"));
    CHECK_FALSE(are_materials_compatible("PC", "PA")); // Different engineering groups
}

TEST_CASE("are_materials_compatible - unknown material compatible with everything", "[filament][database][compat]") {
    CHECK(are_materials_compatible("FooBar", "PLA"));
    CHECK(are_materials_compatible("PLA", "FooBar"));
    CHECK(are_materials_compatible("FooBar", "ABS"));
}

TEST_CASE("are_materials_compatible - both unknown returns true", "[filament][database][compat]") {
    CHECK(are_materials_compatible("FooBar", "BazQux"));
}

// ============================================================================
// get_compatibility_group tests
// ============================================================================

TEST_CASE("get_compatibility_group - known material returns group", "[filament][database][compat]") {
    CHECK(std::string_view(get_compatibility_group("PLA")) == "PLA");
    CHECK(std::string_view(get_compatibility_group("PETG")) == "PETG");
    CHECK(std::string_view(get_compatibility_group("ABS")) == "ABS_ASA");
    CHECK(std::string_view(get_compatibility_group("PA")) == "PA");
    CHECK(std::string_view(get_compatibility_group("TPU")) == "TPU");
    CHECK(std::string_view(get_compatibility_group("PC")) == "PC");
    CHECK(std::string_view(get_compatibility_group("PEEK")) == "HIGH_TEMP");
}

TEST_CASE("get_compatibility_group - unknown material returns nullptr", "[filament][database][compat]") {
    CHECK(get_compatibility_group("FooBar") == nullptr);
    CHECK(get_compatibility_group("UnknownMaterial") == nullptr);
}

// ============================================================================
// get_drying_presets_by_group tests
// ============================================================================

TEST_CASE("get_drying_presets_by_group - returns non-empty vector", "[filament][database][drying]") {
    auto presets = get_drying_presets_by_group();
    CHECK_FALSE(presets.empty());
}

TEST_CASE("get_drying_presets_by_group - contains expected groups", "[filament][database][drying]") {
    auto presets = get_drying_presets_by_group();

    auto has_group = [&presets](std::string_view name) {
        for (const auto& p : presets) {
            if (std::string_view(p.name) == name) {
                return true;
            }
        }
        return false;
    };

    CHECK(has_group("PLA"));
    CHECK(has_group("PETG"));
    CHECK(has_group("ABS_ASA"));
    CHECK(has_group("PC"));
    CHECK(has_group("PA"));
    CHECK(has_group("TPU"));
    CHECK(has_group("HIGH_TEMP"));
}

TEST_CASE("get_drying_presets_by_group - each preset has reasonable values", "[filament][database][drying]") {
    auto presets = get_drying_presets_by_group();

    for (const auto& preset : presets) {
        INFO("Checking preset: " << preset.name);
        CHECK(preset.temp_c > 0);
        CHECK(preset.temp_c <= 120); // Reasonable upper bound
        CHECK(preset.time_min > 0);
        CHECK(preset.time_min <= 720); // 12 hours max
    }
}

TEST_CASE("get_drying_presets_by_group - presets have unique groups", "[filament][database][drying]") {
    auto presets = get_drying_presets_by_group();

    for (size_t i = 0; i < presets.size(); i++) {
        for (size_t j = i + 1; j < presets.size(); j++) {
            CHECK(std::string_view(presets[i].name) != std::string_view(presets[j].name));
        }
    }
}

// ============================================================================
// weight_to_length_m tests
// ============================================================================

TEST_CASE("weight_to_length_m - 1kg PLA calculation", "[filament][database][weight]") {
    // 1kg PLA (density 1.24 g/cm³) at 1.75mm diameter
    // Expected: approximately 335m (standard industry value)
    float length = weight_to_length_m(1000.0f, 1.24f, 1.75f);

    // Allow 5% tolerance
    CHECK(length == Approx(335.0f).epsilon(0.05f));
}

TEST_CASE("weight_to_length_m - zero weight returns zero", "[filament][database][weight]") {
    float length = weight_to_length_m(0.0f, 1.24f, 1.75f);
    CHECK(length == 0.0f);
}

TEST_CASE("weight_to_length_m - different diameters", "[filament][database][weight]") {
    // 2.85mm filament should give shorter length for same weight
    float length_175 = weight_to_length_m(1000.0f, 1.24f, 1.75f);
    float length_285 = weight_to_length_m(1000.0f, 1.24f, 2.85f);

    CHECK(length_285 < length_175);
    // 2.85mm is ~1.63x the diameter, so area is ~2.65x larger, length should be ~2.65x shorter
    CHECK(length_175 / length_285 == Approx(2.65f).epsilon(0.05f));
}

TEST_CASE("weight_to_length_m - different densities", "[filament][database][weight]") {
    // Higher density = shorter length for same weight
    float length_pla = weight_to_length_m(1000.0f, 1.24f); // PLA
    float length_abs = weight_to_length_m(1000.0f, 1.04f); // ABS

    CHECK(length_abs > length_pla); // ABS is less dense, more length per kg
}

// ============================================================================
// MaterialInfo helper method tests
// ============================================================================

TEST_CASE("MaterialInfo::needs_enclosure - PLA does not need enclosure", "[filament][database][helpers]") {
    auto pla = find_material("PLA");
    REQUIRE(pla.has_value());
    CHECK(pla->chamber_temp_c == 0);
    CHECK_FALSE(pla->needs_enclosure());
}

TEST_CASE("MaterialInfo::needs_enclosure - ABS needs enclosure", "[filament][database][helpers]") {
    auto abs = find_material("ABS");
    REQUIRE(abs.has_value());
    CHECK(abs->chamber_temp_c == 50);
    CHECK(abs->needs_enclosure());
}

TEST_CASE("MaterialInfo::needs_enclosure - PETG does not need enclosure", "[filament][database][helpers]") {
    auto petg = find_material("PETG");
    REQUIRE(petg.has_value());
    CHECK_FALSE(petg->needs_enclosure());
}

TEST_CASE("MaterialInfo::needs_enclosure - PC needs enclosure", "[filament][database][helpers]") {
    auto pc = find_material("PC");
    REQUIRE(pc.has_value());
    CHECK(pc->needs_enclosure());
}

TEST_CASE("MaterialInfo::needs_drying - PLA needs drying", "[filament][database][helpers]") {
    auto pla = find_material("PLA");
    REQUIRE(pla.has_value());
    CHECK(pla->dry_temp_c == 45);
    CHECK(pla->needs_drying());
}

TEST_CASE("MaterialInfo::needs_drying - all materials need drying", "[filament][database][helpers]") {
    // All materials in our database have dry_temp_c > 0
    for (const auto& mat : MATERIALS) {
        INFO("Checking material: " << mat.name);
        CHECK(mat.needs_drying());
    }
}

TEST_CASE("MaterialInfo::nozzle_recommended - returns midpoint", "[filament][database][helpers]") {
    auto pla = find_material("PLA");
    REQUIRE(pla.has_value());

    // PLA: 190-220, midpoint = 205
    CHECK(pla->nozzle_recommended() == (190 + 220) / 2);
    CHECK(pla->nozzle_recommended() == 205);
}

TEST_CASE("MaterialInfo::nozzle_recommended - ABS midpoint", "[filament][database][helpers]") {
    auto abs = find_material("ABS");
    REQUIRE(abs.has_value());

    // ABS: 240-270, midpoint = 255
    CHECK(abs->nozzle_recommended() == (240 + 270) / 2);
    CHECK(abs->nozzle_recommended() == 255);
}

TEST_CASE("MaterialInfo::nozzle_recommended - PEEK high temp", "[filament][database][helpers]") {
    auto peek = find_material("PEEK");
    REQUIRE(peek.has_value());

    // PEEK: 370-420, midpoint = 395
    CHECK(peek->nozzle_recommended() == (370 + 420) / 2);
    CHECK(peek->nozzle_recommended() == 395);
}

// ============================================================================
// Additional coverage tests
// ============================================================================

TEST_CASE("get_materials_by_category - Standard category", "[filament][database]") {
    auto materials = get_materials_by_category("Standard");
    CHECK_FALSE(materials.empty());

    bool has_pla = false;
    bool has_petg = false;
    for (const auto& mat : materials) {
        if (std::string_view(mat.name) == "PLA") has_pla = true;
        if (std::string_view(mat.name) == "PETG") has_petg = true;
    }
    CHECK(has_pla);
    CHECK(has_petg);
}

TEST_CASE("get_categories - returns all categories", "[filament][database]") {
    auto categories = get_categories();
    CHECK_FALSE(categories.empty());

    auto has_category = [&categories](std::string_view name) {
        for (const auto* cat : categories) {
            if (std::string_view(cat) == name) return true;
        }
        return false;
    };

    CHECK(has_category("Standard"));
    CHECK(has_category("Engineering"));
    CHECK(has_category("Flexible"));
    CHECK(has_category("Support"));
    CHECK(has_category("Specialty"));
    CHECK(has_category("High-Temp"));
}

TEST_CASE("get_all_material_names - returns all materials", "[filament][database]") {
    auto names = get_all_material_names();
    CHECK(names.size() == MATERIAL_COUNT);
}

TEST_CASE("MATERIAL_COUNT matches array size", "[filament][database]") {
    size_t count = 0;
    for (const auto& mat : MATERIALS) {
        (void)mat;
        count++;
    }
    CHECK(count == MATERIAL_COUNT);
}
