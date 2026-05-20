#pragma once

#include "types.hpp"
#include "vpk_writer.hpp"

#include <filesystem>
#include <vector>

namespace dppbot {

std::vector<std::filesystem::path> DiscoverVpkPacks(const std::filesystem::path& root);
PackScanResult ScanPackHeroes(const std::filesystem::path& packPath);
LibrarySummary BuildLibrarySummary(const std::filesystem::path& root);
MultiPackHeroReport AnalyzeHeroAcrossPacks(const std::vector<std::filesystem::path>& packs, const std::string& hero);
std::filesystem::path ExportMergedHeroPack(const std::vector<std::filesystem::path>& packs, const std::string& hero, const std::filesystem::path& outputPath);

class PackLibrary {
public:
    void AddPack(const std::filesystem::path& packPath);
    std::vector<std::pair<std::string, std::size_t>> GetAllHeroes() const;
    MultiPackHeroReport AnalyzeHero(const std::string& hero) const;

private:
    std::vector<std::filesystem::path> packs_;
};

}  // namespace dppbot
