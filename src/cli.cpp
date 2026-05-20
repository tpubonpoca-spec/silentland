#include "cli.hpp"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace dppbot {

namespace {

bool IsFlag(std::string_view value, std::string_view shortName, std::string_view longName) {
    return value == shortName || value == longName;
}

std::string RequireValue(int argc, char** argv, int& index, std::string_view flag) {
    if (index + 1 >= argc) {
        throw std::runtime_error("Missing value after flag: " + std::string(flag));
    }
    ++index;
    return argv[index];
}

}  // namespace

CliOptions ParseCli(int argc, char** argv) {
    CliOptions options;

    if (argc <= 1) {
        return options;
    }

    int index = 1;
    const std::string first = argv[index];
    if (first == "scan" || first == "extract" || first == "merge" || first == "analyze") {
        options.command = first;
        ++index;
    }

    for (; index < argc; ++index) {
        const std::string_view arg = argv[index];
        if (IsFlag(arg, "-v", "--vpk")) {
            options.vpkPath = RequireValue(argc, argv, index, arg);
        } else if (IsFlag(arg, "-p", "--packs")) {
            // Multi-pack mode: comma-separated list of VPK paths
            std::string packsStr = RequireValue(argc, argv, index, arg);
            std::size_t start = 0;
            std::size_t end = packsStr.find(',');
            while (end != std::string::npos) {
                options.vpkPaths.push_back(packsStr.substr(start, end - start));
                start = end + 1;
                end = packsStr.find(',', start);
            }
            options.vpkPaths.push_back(packsStr.substr(start));
        } else if (IsFlag(arg, "-h", "--hero")) {
            options.hero = RequireValue(argc, argv, index, arg);
        } else if (IsFlag(arg, "-o", "--output")) {
            options.outputDirectory = RequireValue(argc, argv, index, arg);
        } else if (arg == "--site-metadata") {
            options.withSiteMetadata = true;
        } else if (arg == "--json") {
            options.jsonOutput = true;
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--help" || arg == "-?") {
            PrintUsage();
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + std::string(arg));
        }
    }

    if (options.command == "merge" || options.command == "analyze") {
        if (options.vpkPaths.empty()) {
            throw std::runtime_error("Multi-pack commands require --packs <path1,path2,...>.");
        }
        if (options.command == "merge") {
            if (options.hero.empty()) {
                throw std::runtime_error("Merge command requires --hero <hero_slug>.");
            }
            if (options.outputDirectory.empty()) {
                throw std::runtime_error("Merge command requires --output <directory>.");
            }
        }
    } else {
        if (options.vpkPath.empty()) {
            throw std::runtime_error("You must pass --vpk <path to *_dir.vpk>.");
        }
    }

    if (options.command == "extract") {
        if (options.outputDirectory.empty()) {
            throw std::runtime_error("Extract mode requires --output <directory>.");
        }
    }

    return options;
}

void PrintUsage() {
    std::cout
        << "Usage:\n"
        << "  dppbotcpp scan --vpk <pak04_dir.vpk> [--site-metadata] [--verbose]\n"
        << "  dppbotcpp extract --vpk <pak04_dir.vpk> --output <folder> [--hero <hero_slug>] [--site-metadata]\n"
        << "  dppbotcpp analyze --packs <pack1.vpk,pack2.vpk,...> [--hero <hero_slug>] [--json] [--verbose]\n"
        << "  dppbotcpp merge --packs <pack1.vpk,pack2.vpk,...> --hero <hero_slug> --output <folder> [--json]\n\n"
        << "Commands:\n"
        << "  scan     - Analyze a single VPK pack and list heroes\n"
        << "  extract  - Extract hero pack from a single VPK\n"
        << "  analyze  - Analyze multiple VPK packs and show conflicts/coverage\n"
        << "  merge    - Merge hero content from multiple VPK packs into one\n\n"
        << "Examples:\n"
        << "  dppbotcpp scan --vpk C:\\Users\\PMC\\Desktop\\pak04_dir.vpk\n"
        << "  dppbotcpp extract --vpk C:\\Users\\PMC\\Desktop\\pak04_dir.vpk --output C:\\mods\\selected_pack\n"
        << "  dppbotcpp extract --vpk C:\\Users\\PMC\\Desktop\\pak04_dir.vpk --hero shadow_fiend --output C:\\mods\\sf_pack\n"
        << "  dppbotcpp analyze --packs pack1_dir.vpk,pack2_dir.vpk --hero shadow_fiend --json\n"
        << "  dppbotcpp merge --packs pack1_dir.vpk,pack2_dir.vpk --hero shadow_fiend --output C:\\mods\\merged\n";
}

}  // namespace dppbot
