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
    const std::string manifestText = manifest.str();
    entries.push_back({"manifest.json", std::vector<std::uint8_t>(manifestText.begin(), manifestText.end())});

    VpkWriter writer;
    std::filesystem::path finalPath = outputPath;
    if (finalPath.extension() != ".vpk") {
        finalPath /= hero + "_merged_dir.vpk";
    }
    writer.Write(finalPath, entries);

    // Export JSON report alongside VPK
    std::filesystem::path jsonReportPath = finalPath;
    jsonReportPath.replace_extension(".json");

    std::ostringstream jsonReport;
    jsonReport << "{\n";
    jsonReport << "  \"export_info\": {\n";
    jsonReport << "    \"hero\": \"" << hero << "\",\n";
    jsonReport << "    \"vpk_file\": \"" << finalPath.filename().string() << "\",\n";
    jsonReport << "    \"export_date\": \"2026-05-20\",\n";
    jsonReport << "    \"merge_type\": \"multi_pack\"\n";
    jsonReport << "  },\n";
    jsonReport << "  \"source_packs\": [\n";
    for (std::size_t i = 0; i < sourcePacks.size(); ++i) {
        jsonReport << "    {\"name\": \"" << sourcePacks[i] << "\", \"priority\": " << (i + 1) << "}";
        if (i + 1 < sourcePacks.size()) jsonReport << ",";
        jsonReport << "\n";
    }
    jsonReport << "  ],\n";
    jsonReport << "  \"statistics\": {\n";
    jsonReport << "    \"total_seed_files\": " << totalSeedFiles << ",\n";
    jsonReport << "    \"total_included_files\": " << totalIncludedFiles << ",\n";
    jsonReport << "    \"merged_unique_files\": " << merged.size() << ",\n";
    jsonReport << "    \"conflicts_count\": " << conflicts.size() << "\n";
    jsonReport << "  },\n";
    jsonReport << "  \"conflicts\": [\n";
    for (std::size_t i = 0; i < conflicts.size(); ++i) {
        jsonReport << "    \"" << conflicts[i] << "\"";
        if (i + 1 < conflicts.size()) jsonReport << ",";
        jsonReport << "\n";
    }
    jsonReport << "  ],\n";
    jsonReport << "  \"preview_assets\": [\n";
    idx = 0;
    for (const auto& preview : allPreviewAssets) {
        jsonReport << "    \"" << preview << "\"";
        if (++idx < allPreviewAssets.size()) jsonReport << ",";
        jsonReport << "\n";
    }
    jsonReport << "  ],\n";
```

## 태그
- ui
- cpp