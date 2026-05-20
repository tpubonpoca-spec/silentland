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
    if (first == "scan" || first == "extract") {
        options.command = first;
        ++index;
    }

    for (; index < argc; ++index) {
        const std::string_view arg = argv[index];
        if (IsFlag(arg, "-v", "--vpk")) {
            options.vpkPath = RequireValue(argc, argv, index, arg);
        } else if (IsFlag(arg, "-h", "--hero")) {
            options.hero = RequireValue(argc, argv, index, arg);
        } else if (IsFlag(arg, "-o", "--output")) {
            options.outputDirectory = RequireValue(argc, argv, index, arg);
        } else if (arg == "--site-metadata") {
            options.withSiteMetadata = true;
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--help" || arg == "-?") {
            PrintUsage();
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown argument: " + std::string(arg));
        }
    }

    if (options.vpkPath.empty()) {
        throw std::runtime_error("You must pass --vpk <path to *_dir.vpk>.");
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
        << "  dppbotcpp extract --vpk <pak04_dir.vpk> --output <folder> [--hero <hero_slug>] [--site-metadata]\n\n"
        << "Examples:\n"
        << "  dppbotcpp scan --vpk C:\\Users\\PMC\\Desktop\\pak04_dir.vpk\n"
        << "  dppbotcpp extract --vpk C:\\Users\\PMC\\Desktop\\pak04_dir.vpk --output C:\\mods\\selected_pack\n"
        << "  dppbotcpp extract --vpk C:\\Users\\PMC\\Desktop\\pak04_dir.vpk --hero shadow_fiend --output C:\\mods\\sf_pack\n";
}

}  // namespace dppbot
