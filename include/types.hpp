#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace dppbot {

struct PreviewAsset {
    std::string path;
    std::string kind;
    std::string note;
};

struct ReplacementHint {
    std::string category;
    std::string targetPath;
    std::string note;
};

struct VpkEntry {
    std::string path;
    std::uint32_t crc32 = 0;
    std::uint16_t preloadBytes = 0;
    std::uint16_t archiveIndex = 0;
    std::uint32_t offset = 0;
    std::uint32_t length = 0;
    std::vector<std::uint8_t> preloadData;
};

struct ScanSummary {
    std::string normalizedHero;
    std::string sourcePackName;
    std::vector<std::string> aliases;
    std::vector<std::string> seedFiles;
    std::vector<std::string> includedFiles;
    std::unordered_map<std::string, std::size_t> filesByRoot;
    std::unordered_map<std::string, std::size_t> filesByExtension;
    std::vector<std::string> models;
    std::vector<std::string> materials;
    std::vector<std::string> particles;
    std::vector<std::string> sounds;
    std::vector<std::string> uiAssets;
    std::vector<std::string> itemVisuals;
    std::vector<PreviewAsset> previewAssets;
    std::vector<ReplacementHint> replacementHints;
    std::vector<std::string> notes;
};

struct SiteHeroInfo {
    std::string slug;
    std::string title;
    std::string url;
};

struct SiteCatalog {
    std::vector<SiteHeroInfo> heroes;
    std::string sourceUrl;
    std::string warning;
};

struct CliOptions {
    std::filesystem::path vpkPath;
    std::vector<std::filesystem::path> vpkPaths;  // For multi-pack commands
    std::filesystem::path outputDirectory;
    std::string command = "scan";
    std::string hero;
    bool withSiteMetadata = false;
    bool verbose = false;
    bool jsonOutput = false;  // For machine-readable output
};

struct PackScanResult {
    std::filesystem::path packPath;
    std::vector<std::string> heroes;
    std::string error;
};

struct LibrarySummary {
    std::vector<PackScanResult> packs;
    std::vector<std::pair<std::string, std::size_t>> heroCoverage;
};

struct MultiPackHeroReport {
    std::string hero;
    std::vector<ScanSummary> sources;
    std::vector<PreviewAsset> previewAssets;
    std::vector<ReplacementHint> replacementHints;
    std::vector<std::string> conflicts;
    std::unordered_map<std::string, std::size_t> filesByRoot;
    std::unordered_map<std::string, std::size_t> filesByExtension;
    std::size_t totalSeedFiles = 0;
    std::size_t totalIncludedFiles = 0;
    std::size_t mergedUniqueFiles = 0;
};

}  // namespace dppbot
