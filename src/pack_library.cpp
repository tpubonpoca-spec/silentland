#include "pack_library.hpp"

#include "mod_analyzer.hpp"
#include "vpk_archive.hpp"

#include <algorithm>
#include <set>
#include <unordered_set>
#include <unordered_map>

namespace dppbot {

std::vector<std::filesystem::path> DiscoverVpkPacks(const std::filesystem::path& root) {
    std::vector<std::filesystem::path> packs;
    std::error_code error;
    if (!std::filesystem::exists(root, error)) {
        return packs;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root, error)) {
        if (error) {
            break;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto filename = entry.path().filename().string();
        if (filename.size() >= 8 && filename.rfind("_dir.vpk") == filename.size() - 8) {
            packs.push_back(entry.path());
        }
    }

    std::sort(packs.begin(), packs.end());
    return packs;
}

PackScanResult ScanPackHeroes(const std::filesystem::path& packPath) {
    PackScanResult result;
    result.packPath = packPath;

    try {
        VpkArchive archive;
        archive.Load(packPath);
        ModAnalyzer analyzer(archive);
        result.heroes = analyzer.DetectHeroes();
    } catch (const std::exception& ex) {
        result.error = ex.what();
    }

    return result;
}

LibrarySummary BuildLibrarySummary(const std::filesystem::path& root) {
    LibrarySummary summary;
    std::unordered_map<std::string, std::size_t> heroCoverage;

    for (const auto& pack : DiscoverVpkPacks(root)) {
        PackScanResult result = ScanPackHeroes(pack);
        for (const auto& hero : result.heroes) {
            heroCoverage[hero]++;
        }
        summary.packs.push_back(std::move(result));
    }

    summary.heroCoverage.assign(heroCoverage.begin(), heroCoverage.end());
    std::sort(summary.heroCoverage.begin(), summary.heroCoverage.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) {
            return left.second > right.second;
        }
        return left.first < right.first;
    });

    return summary;
}

MultiPackHeroReport AnalyzeHeroAcrossPacks(const std::vector<std::filesystem::path>& packs, const std::string& hero) {
    MultiPackHeroReport report;
    report.hero = hero;
    std::unordered_set<std::string> previewSeen;
    std::unordered_set<std::string> replacementSeen;

    for (const auto& pack : packs) {
        VpkArchive archive;
        archive.Load(pack);
        ModAnalyzer analyzer(archive);
        ScanSummary summary = analyzer.BuildHeroPack(hero);
        report.totalSeedFiles += summary.seedFiles.size();
        report.totalIncludedFiles += summary.includedFiles.size();

        for (const auto& [root, count] : summary.filesByRoot) {
            report.filesByRoot[root] += count;
        }
        for (const auto& [ext, count] : summary.filesByExtension) {
            report.filesByExtension[ext] += count;
        }
        for (const auto& preview : summary.previewAssets) {
            if (previewSeen.insert(preview.path).second) {
                report.previewAssets.push_back(preview);
            }
        }
        for (const auto& replacement : summary.replacementHints) {
            const std::string key = replacement.category + "|" + replacement.targetPath;
            if (replacementSeen.insert(key).second) {
                report.replacementHints.push_back(replacement);
            }
        }
        report.sources.push_back(std::move(summary));
    }

    std::unordered_map<std::string, std::string> ownerByPath;
    std::unordered_set<std::string> uniqueFiles;
    for (const auto& summary : report.sources) {
        for (const auto& path : summary.includedFiles) {
            uniqueFiles.insert(path);
            const auto it = ownerByPath.find(path);
            if (it == ownerByPath.end()) {
                ownerByPath[path] = summary.sourcePackName;
            } else if (it->second != summary.sourcePackName) {
                report.conflicts.push_back(path + " overridden by " + summary.sourcePackName + " (was " + it->second + ")");
                it->second = summary.sourcePackName;
            }
        }
    }
    report.mergedUniqueFiles = uniqueFiles.size();
    std::sort(report.conflicts.begin(), report.conflicts.end());
    report.conflicts.erase(std::unique(report.conflicts.begin(), report.conflicts.end()), report.conflicts.end());
    return report;
}

std::filesystem::path ExportMergedHeroPack(const std::vector<std::filesystem::path>& packs, const std::string& hero, const std::filesystem::path& outputPath) {
    if (packs.empty()) {
        throw std::runtime_error("At least one pack is required for merged export.");
    }

    std::unordered_map<std::string, VpkWriteEntry> merged;
    for (const auto& pack : packs) {
        VpkArchive archive;
        archive.Load(pack);
        ModAnalyzer analyzer(archive);
        const ScanSummary summary = analyzer.BuildHeroPack(hero);
        for (auto& entry : analyzer.BuildPackEntries(summary)) {
            merged[entry.path] = std::move(entry);
        }
    }

    std::vector<VpkWriteEntry> entries;
    entries.reserve(merged.size());
    for (auto& [path, entry] : merged) {
        entries.push_back(std::move(entry));
    }

    VpkWriter writer;
    std::filesystem::path finalPath = outputPath;
    if (finalPath.extension() != ".vpk") {
        finalPath /= hero + "_merged_dir.vpk";
    }
    writer.Write(finalPath, entries);
    return finalPath;
}

}  // namespace dppbot
