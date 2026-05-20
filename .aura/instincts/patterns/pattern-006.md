---
id: pattern-006
category: test
language: cpp
score: 50
tags: [test, cpp]
---

## 컨텍스트
파일: main.cpp (Edit 완료)

## 핵심 코드
```cpp
            if (options.command == "analyze") {
                if (options.hero.empty()) {
                    // Show all heroes across all packs
                    const auto allHeroes = library.GetAllHeroes();
                    std::cout << "Герои найдены в " << options.vpkPaths.size() << " паках:\n";
                    for (const auto& [hero, count] : allHeroes) {
                        std::cout << "  - " << hero << " (в " << count << " паке(ах))\n";
                    }
                } else {
                    // Analyze specific hero
                    MultiPackHeroReport report = library.AnalyzeHero(options.hero);
                    PrintMultiPackAnalysis(report, options.jsonOutput);
                }
                return 0;
            }

            if (options.command == "merge") {
                std::cout << "Объединение героя '" << options.hero << "' из " << options.vpkPaths.size() << " паков...\n";
                const auto exportedPath = ExportMergedHeroPack(options.vpkPaths, options.hero, options.outputDirectory);
                std::cout << "Объединённый пак экспортирован в: " << exportedPath.string() << "\n";
                std::cout << "JSON отчёт: " << exportedPath.string().substr(0, exportedPath.string().length() - 4) << ".json\n";
                return 0;
            }
```

## 태그
- test
- cpp