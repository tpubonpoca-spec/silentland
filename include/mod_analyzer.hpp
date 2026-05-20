#pragma once

#include "types.hpp"
#include "vpk_archive.hpp"
#include "vpk_writer.hpp"

#include <string>
#include <vector>

namespace dppbot {

class ModAnalyzer {
public:
    explicit ModAnalyzer(const VpkArchive& archive);

    std::vector<std::string> DetectHeroes() const;
    ScanSummary BuildHeroPack(const std::string& hero) const;
    std::vector<VpkWriteEntry> BuildPackEntries(const ScanSummary& summary) const;
    std::filesystem::path ExportPack(const ScanSummary& summary, const std::filesystem::path& outputPathOrDirectory) const;

private:
    const VpkArchive& archive_;
};

}  // namespace dppbot
