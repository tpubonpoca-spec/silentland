---
id: pattern-001
category: ui
language: cpp
score: 50
tags: [ui, cpp]
---

## 컨텍스트
파일: pack_library.cpp (Edit 완료)

## 핵심 코드
```cpp
std::filesystem::path ExportMergedHeroPack(const std::vector<std::filesystem::path>& packs, const std::string& hero, const std::filesystem::path& outputPath) {
    if (packs.empty()) {
        throw std::runtime_error("At least one pack is required for merged export.");
    }

    std::unordered_map<std::string, VpkWriteEntry> merged;
    std::vector<std::string> sourcePacks;
    std::vector<std::string> conflicts;
    std::unordered_map<std::string, std::string> pathOwner;
    std::unordered_set<std::string> allPreviewAssets;
    std::unordered_set<std::string> allReplacementHints;
    std::size_t totalSeedFiles = 0;
    std::size_t totalIncludedFiles = 0;

    for (const auto& pack : packs) {
        VpkArchive archive;
        archive.Load(pack);
        ModAnalyzer analyzer(archive);
        const ScanSummary summary = analyzer.BuildHeroPack(hero);

        sourcePacks.push_back(pack.filename().string());
        totalSeedFiles += summary.seedFiles.size();
        totalIncludedFiles += summary.includedFiles.size();

        for (const auto& preview : summary.previewAssets) {
            allPreviewAssets.insert(preview.path);
        }
        for (const auto& replacement : summary.replacementHints) {
            allReplacementHints.insert(replacement.category + ": " + replacement.targetPath);
        }

        for (auto& entry : analyzer.BuildPackEntries(summary)) {
            if (entry.path == "manifest.json") {
                continue;
            }

            auto it = pathOwner.find(entry.path);
            if (it != pathOwner.end() && it->second != pack.filename().string()) {
                conflicts.push_back(entry.path + " (overridden by " + pack.filename().string() + ", was " + it->second + ")");
            }
            pathOwner[entry.path] = pack.filename().string();
            merged[entry.path] = std::move(entry);
        }
    }

    std::vector<VpkWriteEntry> entries;
    entries.reserve(merged.size() + 1);
    for (auto& [path, entry] : merged) {
        entries.push_back(std::move(entry));
    }
```

## 태그
- ui
- cpp