// Copyright 2025 356C LLC
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "gcode_ops_detector.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// Forward declarations
class MoonrakerAPI;

namespace gcode {

/**
 * @brief RAII wrapper for modified G-code temp files.
 *
 * Automatically tracks temp file state for post-print cleanup.
 * The actual file deletion is done via Moonraker API (since files
 * are on the printer, not local filesystem).
 *
 * Movable but not copyable. Use release() to prevent cleanup
 * if the file should be retained.
 *
 * @code
 * auto temp = modifier.create_skip_copy(original, ops_to_skip);
 * if (temp) {
 *     // Print using temp->moonraker_path()
 *     // After print completes, temp destructor triggers cleanup callback
 * }
 * @endcode
 */
class TempGCodeFile {
public:
    using CleanupCallback = std::function<void(const std::string& moonraker_path)>;

    /**
     * @brief Construct a temp file handle
     *
     * @param moonraker_path Path on Moonraker server (e.g., ".helix_temp/original.gcode")
     * @param original_filename Original filename for job history patching
     * @param cleanup_callback Called on destruction to delete the temp file
     */
    TempGCodeFile(std::string moonraker_path, std::string original_filename,
                  CleanupCallback cleanup_callback);

    ~TempGCodeFile();

    // Move-only semantics
    TempGCodeFile(TempGCodeFile&& other) noexcept;
    TempGCodeFile& operator=(TempGCodeFile&& other) noexcept;
    TempGCodeFile(const TempGCodeFile&) = delete;
    TempGCodeFile& operator=(const TempGCodeFile&) = delete;

    /**
     * @brief Get the path to use with Moonraker start_print
     * @return Path relative to gcodes root (e.g., ".helix_temp/original.gcode")
     */
    [[nodiscard]] const std::string& moonraker_path() const { return moonraker_path_; }

    /**
     * @brief Get the original filename for job history patching
     * @return Original filename that should appear in print history
     */
    [[nodiscard]] const std::string& original_filename() const { return original_filename_; }

    /**
     * @brief Release ownership - prevents cleanup on destruction
     *
     * Call this if you want to keep the temp file (e.g., for debugging).
     * After calling release(), the destructor will not delete the file.
     */
    void release();

    /**
     * @brief Check if this handle owns the file
     * @return true if destructor will trigger cleanup
     */
    [[nodiscard]] bool owns_file() const { return owns_file_; }

private:
    std::string moonraker_path_;
    std::string original_filename_;
    CleanupCallback cleanup_callback_;
    bool owns_file_ = true;
};

/**
 * @brief Result of creating a skip copy
 */
struct SkipCopyResult {
    std::unique_ptr<TempGCodeFile> temp_file;  ///< RAII handle for the temp file
    std::vector<OperationType> skipped_ops;    ///< Operations that were commented out
    size_t lines_modified = 0;                  ///< Number of lines modified
};

/**
 * @brief Configuration for file modification behavior
 */
struct ModifierConfig {
    std::string temp_dir = ".helix_temp";  ///< Subdirectory for temp files (under gcodes/)
    std::string skip_prefix = "; HELIX_SKIP: ";  ///< Prefix for commented-out lines
    bool add_header_comment = true;  ///< Add comment at top explaining modifications
};

/**
 * @brief Creates modified G-code files for skip operations.
 *
 * When a user wants to skip an operation that exists in their G-code file
 * (e.g., disable bed leveling that's embedded in start gcode), this class:
 *
 * 1. Reads the original file from the printer via Moonraker
 * 2. Creates a modified copy with detected operations commented out
 * 3. Uploads the modified copy to a temp directory
 * 4. Returns an RAII handle that auto-deletes the temp file
 *
 * Thread-safe for concurrent use with different files.
 *
 * @code
 * GCodeFileModifier modifier(moonraker_api, config);
 *
 * // Create temp file with bed leveling skipped
 * auto result = modifier.create_skip_copy(
 *     "my_print.gcode",
 *     {detected_bed_level_op},
 *     [](auto&&) { spdlog::info("Skip copy created"); },
 *     [](auto& err) { spdlog::error("Failed: {}", err.message); });
 * @endcode
 */
class GCodeFileModifier {
public:
    using SuccessCallback = std::function<void(SkipCopyResult result)>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    /**
     * @brief Construct with Moonraker API reference
     *
     * @param api Reference to MoonrakerAPI (must remain valid for modifier lifetime)
     * @param config Optional configuration
     */
    explicit GCodeFileModifier(MoonrakerAPI& api, const ModifierConfig& config = {});

    // Non-copyable, movable
    GCodeFileModifier(const GCodeFileModifier&) = delete;
    GCodeFileModifier& operator=(const GCodeFileModifier&) = delete;
    GCodeFileModifier(GCodeFileModifier&&) = default;
    GCodeFileModifier& operator=(GCodeFileModifier&&) = default;
    ~GCodeFileModifier() = default;

    /**
     * @brief Create a modified copy with operations commented out
     *
     * This is an asynchronous operation that:
     * 1. Downloads the original file from the printer
     * 2. Comments out lines matching the specified operations
     * 3. Uploads the modified file to the temp directory
     * 4. Returns an RAII handle for automatic cleanup
     *
     * @param original_path Path to original file (relative to gcodes root)
     * @param ops_to_skip Operations to comment out
     * @param on_success Callback with RAII temp file handle
     * @param on_error Error callback
     */
    void create_skip_copy(const std::string& original_path,
                          const std::vector<DetectedOperation>& ops_to_skip,
                          SuccessCallback on_success, ErrorCallback on_error);

    /**
     * @brief Ensure the temp directory exists
     *
     * Creates .helix_temp directory if it doesn't exist.
     * Called automatically by create_skip_copy().
     *
     * @param on_success Success callback
     * @param on_error Error callback
     */
    void ensure_temp_directory(std::function<void()> on_success, ErrorCallback on_error);

    /**
     * @brief Clean up all temp files in the temp directory
     *
     * Useful for startup cleanup to remove orphaned temp files
     * from crashes or unexpected shutdowns.
     *
     * @param on_success Success callback (called after deletion attempts)
     * @param on_error Error callback (called if listing fails)
     */
    void cleanup_all_temp_files(std::function<void(int deleted_count)> on_success,
                                 ErrorCallback on_error);

    /**
     * @brief Get the current configuration
     */
    [[nodiscard]] const ModifierConfig& config() const { return config_; }

private:
    /**
     * @brief Generate modified content with operations commented out
     *
     * @param original_content Original G-code content
     * @param ops_to_skip Operations to comment out
     * @return Modified content and count of modified lines
     */
    std::pair<std::string, size_t> generate_modified_content(
        const std::string& original_content,
        const std::vector<DetectedOperation>& ops_to_skip) const;

    /**
     * @brief Generate header comment for modified file
     */
    std::string generate_header_comment(const std::string& original_filename,
                                         const std::vector<DetectedOperation>& ops_to_skip) const;

    /**
     * @brief Delete a temp file via Moonraker
     */
    void delete_temp_file(const std::string& moonraker_path);

    MoonrakerAPI& api_;
    ModifierConfig config_;
};

/**
 * @brief Utility to patch job history after printing a temp file
 *
 * After a print completes using a temp file, this updates the job
 * history to show the original filename instead of the temp file path.
 *
 * @code
 * // After print completes
 * JobHistoryPatcher patcher(api);
 * patcher.patch_latest_job(temp_file.original_filename(),
 *     []() { spdlog::info("History patched"); },
 *     [](auto& err) { spdlog::warn("Failed to patch: {}", err); });
 * @endcode
 */
class JobHistoryPatcher {
public:
    using SuccessCallback = std::function<void()>;
    using ErrorCallback = std::function<void(const std::string& error)>;

    explicit JobHistoryPatcher(MoonrakerAPI& api);

    /**
     * @brief Patch the most recent job to show a different filename
     *
     * Queries the job history for the latest job and updates its
     * filename field to show the original name.
     *
     * @param original_filename Filename to show in history
     * @param on_success Success callback
     * @param on_error Error callback
     */
    void patch_latest_job(const std::string& original_filename, SuccessCallback on_success,
                          ErrorCallback on_error);

    /**
     * @brief Patch a specific job by ID
     *
     * @param job_id Moonraker job ID
     * @param original_filename Filename to show in history
     * @param on_success Success callback
     * @param on_error Error callback
     */
    void patch_job(const std::string& job_id, const std::string& original_filename,
                   SuccessCallback on_success, ErrorCallback on_error);

private:
    MoonrakerAPI& api_;
};

}  // namespace gcode
