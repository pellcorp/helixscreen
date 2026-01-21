// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "print_file_data.h"
#include "print_history_data.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace helix::ui {

/**
 * @brief Aggregated history statistics for a single filename.
 */
struct FileHistoryStats {
    int success_count = 0;
    int failure_count = 0;
    PrintJobStatus last_status = PrintJobStatus::UNKNOWN;
    std::string uuid;
    size_t size_bytes = 0;
};

/**
 * @brief Merges print history data into file lists.
 *
 * Provides static utilities to update PrintFileData entries with their
 * print history status (COMPLETED, FAILED, CURRENTLY_PRINTING, etc.)
 */
class PrintSelectHistoryIntegration {
public:
    /**
     * @brief Merge history stats into file list, updating status fields.
     * @param files File list to update (modified in place)
     * @param stats_by_filename Map of basename → aggregated stats
     * @param current_print_filename Currently printing file (empty if none)
     */
    static void merge_history_into_files(
        std::vector<PrintFileData>& files,
        const std::unordered_map<std::string, FileHistoryStats>& stats_by_filename,
        const std::string& current_print_filename);

    /**
     * @brief Extract basename from a path (strips directory prefix).
     * @param path Full path or filename
     * @return Basename portion after last '/'
     */
    static std::string extract_basename(const std::string& path);
};

} // namespace helix::ui
