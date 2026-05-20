#include "cli.hpp"
#include "mod_analyzer.hpp"
#include "site_metadata.hpp"
#include "vpk_archive.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace dppbot {

namespace {

std::string PromptHeroSelection(const std::vector<std::string>& heroes) {
    if (heroes.empty()) {
        throw std::runtime_error("No hero-like groups were detected in this VPK.");
    }

    std::cout << "Choose hero to export:\n";
    for (std::size_t index = 0; index < heroes.size(); ++index) {
        std::cout << "  " << (index + 1) << ". " << heroes[index] << "\n";
    }
    std::cout << "Enter number: ";

    std::size_t choice = 0;
    if (!(std::cin >> choice) || choice == 0 || choice > heroes.size()) {
        throw std::runtime_error("Invalid hero selection.");
    }
    return heroes[choice - 1];
}

void PrintSummary(const ScanSummary& summary) {
    std::cout << "Hero: " << summary.normalizedHero << "\n";
    std::cout << "Aliases: ";
    for (std::size_t i = 0; i < summary.aliases.size(); ++i) {
        if (i != 0) {
            std::cout << ", ";
        }
        std::cout << summary.aliases[i];
    }
    std::cout << "\n";
    std::cout << "Seed files: " << summary.seedFiles.size() << "\n";
    std::cout << "Included files: " << summary.includedFiles.size() << "\n";

    std::vector<std::pair<std::string, std::size_t>> roots(summary.filesByRoot.begin(), summary.filesByRoot.end());
    std::sort(roots.begin(), roots.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) {
            return left.second > right.second;
        }
        return left.first < right.first;
    });

    std::cout << "Roots:\n";
    for (const auto& [root, count] : roots) {
        std::cout << "  - " << root << ": " << count << "\n";
    }

    if (!summary.notes.empty()) {
        std::cout << "Notes:\n";
        for (const std::string& note : summary.notes) {
            std::cout << "  - " << note << "\n";
        }
    }
}

}  // namespace

}  // namespace dppbot

int main(int argc, char** argv) {
    using namespace dppbot;

    try {
        const CliOptions options = ParseCli(argc, argv);

        VpkArchive archive;
        archive.Load(options.vpkPath);
        ModAnalyzer analyzer(archive);

        if (options.command == "scan") {
            const auto heroes = analyzer.DetectHeroes();
            std::cout << "Loaded entries: " << archive.Entries().size() << "\n";
            std::cout << "Detected heroes:\n";
            for (const std::string& hero : heroes) {
                std::cout << "  - " << hero << "\n";
            }

            if (options.withSiteMetadata) {
                SiteMetadataClient client;
                const SiteCatalog catalog = client.FetchCatalog();
                std::cout << "\nru.dota2changer.com source: " << catalog.sourceUrl << "\n";
                if (!catalog.warning.empty()) {
                    std::cout << "Site warning: " << catalog.warning << "\n";
                } else {
                    std::cout << "Fetched site items: " << catalog.heroes.size() << "\n";
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
        std::cout << "Exported to: " << exportedPath.string() << "\n";

        if (options.withSiteMetadata) {
            SiteMetadataClient client;
            const SiteCatalog catalog = client.FetchCatalog();
            std::cout << "Site source: " << catalog.sourceUrl << "\n";
            if (!catalog.warning.empty()) {
                std::cout << "Site warning: " << catalog.warning << "\n";
            } else {
                std::cout << "Site items parsed: " << catalog.heroes.size() << "\n";
            }
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n\n";
        PrintUsage();
        return 1;
    }
}
