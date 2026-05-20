#include "cli.hpp"
#include "mod_analyzer.hpp"
#include "pack_library.hpp"
#include "site_metadata.hpp"
#include "vpk_archive.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace dppbot {

namespace {

std::string PromptHeroSelection(const std::vector<std::string>& heroes) {
    if (heroes.empty()) {
        throw std::runtime_error("В этом VPK не обнаружено героев.");
    }

    std::cout << "Выберите героя для экспорта:\n";
    for (std::size_t index = 0; index < heroes.size(); ++index) {
        std::cout << "  " << (index + 1) << ". " << heroes[index] << "\n";
    }
    std::cout << "Введите номер: ";

    std::size_t choice = 0;
    if (!(std::cin >> choice) || choice == 0 || choice > heroes.size()) {
        throw std::runtime_error("Неверный выбор героя.");
    }
    return heroes[choice - 1];
}

void PrintSummary(const ScanSummary& summary) {
    std::cout << "Герой: " << summary.normalizedHero << "\n";
    std::cout << "Псевдонимы: ";
    for (std::size_t i = 0; i < summary.aliases.size(); ++i) {
        if (i != 0) {
            std::cout << ", ";
        }
        std::cout << summary.aliases[i];
    }
    std::cout << "\n";
    std::cout << "Исходных файлов: " << summary.seedFiles.size() << "\n";
    std::cout << "Включённых файлов: " << summary.includedFiles.size() << "\n";

    std::vector<std::pair<std::string, std::size_t>> roots(summary.filesByRoot.begin(), summary.filesByRoot.end());
    std::sort(roots.begin(), roots.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) {
            return left.second > right.second;
        }
        return left.first < right.first;
    });

    std::cout << "Корни:\n";
    for (const auto& [root, count] : roots) {
        std::cout << "  - " << root << ": " << count << "\n";
    }

    if (!summary.notes.empty()) {
        std::cout << "Примечания:\n";
        for (const std::string& note : summary.notes) {
            std::cout << "  - " << note << "\n";
        }
    }
}

void PrintMultiPackAnalysis(const MultiPackHeroReport& report, bool jsonOutput) {
    if (jsonOutput) {
        std::cout << "{\n";
        std::cout << "  \"hero\": \"" << report.hero << "\",\n";
        std::cout << "  \"source_packs\": " << report.sources.size() << ",\n";
        std::cout << "  \"total_seed_files\": " << report.totalSeedFiles << ",\n";
        std::cout << "  \"total_included_files\": " << report.totalIncludedFiles << ",\n";
        std::cout << "  \"merged_unique_files\": " << report.mergedUniqueFiles << ",\n";
        std::cout << "  \"conflicts\": " << report.conflicts.size() << "\n";
        std::cout << "}\n";
    } else {
        std::cout << "Мульти-пак анализ для героя: " << report.hero << "\n";
        std::cout << "Исходных паков: " << report.sources.size() << "\n";
        std::cout << "Всего исходных файлов: " << report.totalSeedFiles << "\n";
        std::cout << "Всего включённых файлов: " << report.totalIncludedFiles << "\n";
        std::cout << "Объединённых уникальных файлов: " << report.mergedUniqueFiles << "\n";
        std::cout << "Конфликтов: " << report.conflicts.size() << "\n";

        if (!report.conflicts.empty()) {
            std::cout << "\nДетали конфликтов:\n";
            for (const auto& conflict : report.conflicts) {
                std::cout << "  - " << conflict << "\n";
            }
        }
    }
}

}  // namespace

}  // namespace dppbot

int main(int argc, char** argv) {
    using namespace dppbot;

    try {
        const CliOptions options = ParseCli(argc, argv);

        // Handle multi-pack commands
        if (options.command == "analyze" || options.command == "merge") {
            PackLibrary library;
            for (const auto& packPath : options.vpkPaths) {
                library.AddPack(packPath);
            }

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
        }

        // Handle single-pack commands
        VpkArchive archive;
        archive.Load(options.vpkPath);
        ModAnalyzer analyzer(archive);

        if (options.command == "scan") {
            const auto heroes = analyzer.DetectHeroes();
            std::cout << "Загружено записей: " << archive.Entries().size() << "\n";
            std::cout << "Обнаружено героев:\n";
            for (const std::string& hero : heroes) {
                std::cout << "  - " << hero << "\n";
            }

            if (options.withSiteMetadata) {
                SiteMetadataClient client;
                const SiteCatalog catalog = client.FetchCatalog();
                std::cout << "\nИсточник ru.dota2changer.com: " << catalog.sourceUrl << "\n";
                if (!catalog.warning.empty()) {
                    std::cout << "Предупреждение сайта: " << catalog.warning << "\n";
                } else {
                    std::cout << "Получено элементов сайта: " << catalog.heroes.size() << "\n";
                }
            }
            return 0;
        }

        std::string hero = options.hero;
        if (hero.empty()) {
            hero = PromptHeroSelection(analyzer.DetectHeroes());
        }

        const ScanSummary summary = analyzer.BuildHeroPack(hero);
        const std::filesystem::path exportedPath = analyzer.ExportPack(summary, options.outputDirectory);
        PrintSummary(summary);
        std::cout << "Экспортировано в: " << exportedPath.string() << "\n";

        if (options.withSiteMetadata) {
            SiteMetadataClient client;
            const SiteCatalog catalog = client.FetchCatalog();
            std::cout << "Источник сайта: " << catalog.sourceUrl << "\n";
            if (!catalog.warning.empty()) {
                std::cout << "Предупреждение сайта: " << catalog.warning << "\n";
            } else {
                std::cout << "Разобрано элементов сайта: " << catalog.heroes.size() << "\n";
            }
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка: " << ex.what() << "\n\n";
        PrintUsage();
        return 1;
    }
}
