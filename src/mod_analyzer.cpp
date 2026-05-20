#include "mod_analyzer.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace dppbot {

namespace {

const std::unordered_set<std::string> kStandardRoots = {
    "models", "materials", "particles", "panorama", "resource", "scripts", "soundevents", "sounds"
};

const std::unordered_map<std::string, std::vector<std::string>> kHeroAliasMap = {
    {"shadow_fiend", {"shadow_fiend", "nevermore"}},
    {"drow_ranger", {"drow_ranger", "drow"}},
    {"windranger", {"windranger", "windrunner"}},
    {"templar_assassin", {"templar_assassin", "lanaya"}},
    {"anti_mage", {"anti_mage", "antimage"}},
    {"io", {"io", "wisp"}},
    {"naga_siren", {"naga_siren", "siren"}},
    {"storm_spirit", {"storm_spirit", "stormspirit"}},
    {"outworld_destroyer", {"outworld_destroyer", "obsidian_destroyer"}}
};

const std::unordered_set<std::string> kIgnoredHeroTokens = {
    "selection", "icons", "slot_item_picker_loadout", "loadout", "body", "head", "back", "legs", "weapon"
};

const std::unordered_set<std::string> kKnownHeroes = {
    "abaddon", "alchemist", "ancient_apparition", "anti_mage", "arc_warden", "axe", "bane", "batrider",
    "beastmaster", "bloodseeker", "bounty_hunter", "brewmaster", "bristleback", "broodmother",
    "centaur_warrunner", "chaos_knight", "chen", "clinkz", "clockwerk", "crystal_maiden", "dark_seer",
    "dark_willow", "dawnbreaker", "dazzle", "death_prophet", "disruptor", "doom", "dragon_knight",
    "drow_ranger", "earth_spirit", "earthshaker", "elder_titan", "ember_spirit", "enchantress", "enigma",
    "faceless_void", "grimstroke", "gyrocopter", "hoodwink", "huskar", "invoker", "io", "jakiro",
    "juggernaut", "keeper_of_the_light", "kez", "kunkka", "legion_commander", "leshrac", "lich",
    "lifestealer", "lina", "lion", "lone_druid", "luna", "lycan", "magnus", "marci", "mars", "medusa",
    "meepo", "mirana", "monkey_king", "morphling", "muerta", "naga_siren", "natures_prophet",
    "necrophos", "night_stalker", "nyx_assassin", "ogre_magi", "omniknight", "oracle",
    "outworld_destroyer", "pangolier", "phantom_assassin", "phantom_lancer", "phoenix", "primal_beast",
    "puck", "pudge", "pugna", "queen_of_pain", "razor", "riki", "ringmaster", "rubick", "sand_king",
    "shadow_demon", "shadow_fiend", "shadow_shaman", "silencer", "skywrath_mage", "slardar", "slark",
    "snapfire", "sniper", "spectre", "spirit_breaker", "storm_spirit", "sven", "techies",
    "templar_assassin", "terrorblade", "tidehunter", "timbersaw", "tinker", "tiny", "treant_protector",
    "troll_warlord", "tusk", "underlord", "undying", "ursa", "vengeful_spirit", "venomancer", "viper",
    "visage", "void_spirit", "warlock", "weaver", "windranger", "winter_wyvern", "witch_doctor",
    "wraith_king", "zeus"
};

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string NormalizeHeroInput(std::string value) {
    value = ToLower(std::move(value));
    std::replace(value.begin(), value.end(), '-', '_');
    std::replace(value.begin(), value.end(), ' ', '_');
    return value;
}

std::vector<std::string> SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char ch : path) {
        if (ch == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

std::string ExtensionOf(const std::string& path) {
    const auto pos = path.find_last_of('.');
    if (pos == std::string::npos || pos + 1 >= path.size()) {
        return "(none)";
    }
    return path.substr(pos + 1);
}

std::vector<std::string> BuildHeroAliases(const std::string& hero) {
    const std::string normalized = NormalizeHeroInput(hero);
    if (const auto it = kHeroAliasMap.find(normalized); it != kHeroAliasMap.end()) {
        return it->second;
    }
    return {normalized};
}

std::string CanonicalizeHeroToken(const std::string& hero) {
    const std::string normalized = NormalizeHeroInput(hero);
    for (const auto& [canonical, aliases] : kHeroAliasMap) {
        if (canonical == normalized) {
            return canonical;
        }
        for (const std::string& alias : aliases) {
            if (alias == normalized) {
                return canonical;
            }
        }
    }
    return normalized;
}

bool ContainsHeroToken(const std::string& path, const std::vector<std::string>& aliases) {
    const auto parts = SplitPath(path);
    for (const std::string& part : parts) {
        const std::string normalized = ToLower(part);
        for (const std::string& alias : aliases) {
            if (normalized.find(alias) != std::string::npos || normalized.find("hero_" + alias) != std::string::npos) {
                return true;
            }
        }
    }
    return false;
}

bool LooksLikeAssetPath(const std::string& token) {
    static const std::vector<std::string> extensions = {
        ".vmdl_c", ".vmesh_c", ".vmat_c", ".vtex_c", ".vpcf_c", ".vsnd_c", ".vsndevts_c", ".vxml_c",
        ".vanim_c", ".vcss_c", ".txt", ".webm", ".png", ".jpg", ".vsnap_c", ".db"
    };
    if (token.find('/') == std::string::npos && token.find('\\') == std::string::npos) {
        return false;
    }
    for (const std::string& ext : extensions) {
        if (token.size() >= ext.size() && token.rfind(ext) == token.size() - ext.size()) {
            return true;
        }
    }
    return false;
}

bool EndsWith(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() && value.rfind(suffix) == value.size() - suffix.size();
}

std::string JoinAliases(const std::vector<std::string>& aliases) {
    std::ostringstream out;
    for (std::size_t i = 0; i < aliases.size(); ++i) {
        if (i != 0) {
            out << ", ";
        }
        out << aliases[i];
    }
    return out.str();
}

std::string DescribeReplacement(const std::string& path) {
    if (path.find("models/heroes/") != std::string::npos) {
        return "Replaces hero model geometry or wearable attachment.";
    }
    if (path.find("materials/models/heroes/") != std::string::npos) {
        return "Replaces hero material, tint, or surface texture setup.";
    }
    if (path.find("particles/units/heroes/") != std::string::npos) {
        return "Replaces hero particle effects or spell visuals.";
    }
    if (path.find("soundevents") != std::string::npos || path.find("sounds/") != std::string::npos) {
        return "Replaces hero sound events or audio cues.";
    }
    if (path.find("panorama/") != std::string::npos || path.find("resource/") != std::string::npos) {
        return "Replaces UI, icon, preview, or front-end presentation assets.";
    }
    if (path.find("items/") != std::string::npos) {
        return "Replaces or augments wearable item visuals.";
    }
    return "Custom asset included in the exported hero pack.";
}

void PushLimited(std::vector<std::string>& values, const std::string& value, std::size_t limit = 24) {
    if (values.size() < limit) {
        values.push_back(value);
    }
}

void PushPreview(std::vector<PreviewAsset>& values, const PreviewAsset& value, std::size_t limit = 16) {
    if (values.size() < limit) {
        values.push_back(value);
    }
}

void PushReplacement(std::vector<ReplacementHint>& values, const ReplacementHint& value, std::size_t limit = 32) {
    if (values.size() < limit) {
        values.push_back(value);
    }
}

std::vector<std::string> ExtractReferencedPaths(const std::vector<std::uint8_t>& data) {
    std::vector<std::string> results;
    std::unordered_set<std::string> unique;
    std::string current;

    auto flush = [&]() {
        if (current.size() < 6) {
            current.clear();
            return;
        }
        std::replace(current.begin(), current.end(), '\\', '/');
        current.erase(std::remove(current.begin(), current.end(), '"'), current.end());
        current = ToLower(current);
        if (LooksLikeAssetPath(current) && unique.insert(current).second) {
            results.push_back(current);
        }
        current.clear();
    };

    for (std::uint8_t byte : data) {
        if (byte >= 32 && byte <= 126) {
            current.push_back(static_cast<char>(byte));
        } else {
            flush();
        }
    }
    flush();

    return results;
}

std::string JsonEscape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char ch : value) {
        switch (ch) {
        case '\\': out << "\\\\"; break;
        case '"': out << "\\\""; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 32) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(ch);
            } else {
                out << static_cast<char>(ch);
            }
            break;
        }
    }
    return out.str();
}

}  // namespace

ModAnalyzer::ModAnalyzer(const VpkArchive& archive) : archive_(archive) {}

std::vector<std::string> ModAnalyzer::DetectHeroes() const {
    std::unordered_map<std::string, std::size_t> counts;

    for (const VpkEntry& entry : archive_.Entries()) {
        const auto parts = SplitPath(entry.path);
        for (std::size_t index = 0; index < parts.size(); ++index) {
            const std::string& part = parts[index];
            if (part.rfind("hero_", 0) == 0 && part.size() > 5) {
                counts[CanonicalizeHeroToken(part.substr(5))]++;
            }
            if (index == 2 && parts[index - 1] == "heroes" && parts[index - 2] == "models") {
                counts[CanonicalizeHeroToken(part)]++;
            }
            if (index > 0 && parts[index - 1] == "items") {
                counts[CanonicalizeHeroToken(part)]++;
            }
        }
    }

    std::vector<std::pair<std::string, std::size_t>> ranked(counts.begin(), counts.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto& left, const auto& right) {
        if (left.second != right.second) {
            return left.second > right.second;
        }
        return left.first < right.first;
    });

    std::vector<std::string> heroes;
    for (const auto& [hero, count] : ranked) {
        if (count < 2) {
            continue;
        }
        if (hero.find('.') != std::string::npos) {
            continue;
        }
        if (kIgnoredHeroTokens.count(hero)) {
            continue;
        }
        if (!kKnownHeroes.count(hero)) {
            continue;
        }
        heroes.push_back(hero);
    }
    return heroes;
}

ScanSummary ModAnalyzer::BuildHeroPack(const std::string& hero) const {
    ScanSummary summary;
    summary.normalizedHero = NormalizeHeroInput(hero);
    summary.sourcePackName = archive_.FilePath().filename().string();
    summary.aliases = BuildHeroAliases(summary.normalizedHero);

    std::unordered_set<std::string> selected;
    std::queue<std::string> queue;

    for (const VpkEntry& entry : archive_.Entries()) {
        if (ContainsHeroToken(entry.path, summary.aliases)) {
            if (selected.insert(entry.path).second) {
                queue.push(entry.path);
                summary.seedFiles.push_back(entry.path);
            }
        }
    }

    if (summary.seedFiles.empty()) {
        throw std::runtime_error("No files matched hero '" + hero + "' in the provided VPK.");
    }

    while (!queue.empty()) {
        const std::string currentPath = queue.front();
        queue.pop();
        const VpkEntry* entry = archive_.Find(currentPath);
        if (!entry) {
            continue;
        }

        const auto data = archive_.ReadFile(*entry);
        for (const std::string& referenced : ExtractReferencedPaths(data)) {
            if (const VpkEntry* dependency = archive_.Find(referenced)) {
                if (selected.insert(dependency->path).second) {
                    queue.push(dependency->path);
                }
            }
        }
    }

    summary.includedFiles.assign(selected.begin(), selected.end());
    std::sort(summary.includedFiles.begin(), summary.includedFiles.end());
    std::sort(summary.seedFiles.begin(), summary.seedFiles.end());

    for (const std::string& path : summary.includedFiles) {
        const auto parts = SplitPath(path);
        const std::string root = parts.empty() ? "(root)" : parts.front();
        summary.filesByRoot[root]++;
        summary.filesByExtension[ExtensionOf(path)]++;

        if (path.find("models/") == 0) {
            PushLimited(summary.models, path);
        }
        if (path.find("materials/") == 0 || path.find("/materials/") != std::string::npos) {
            PushLimited(summary.materials, path);
        }
        if (path.find("particles/") == 0) {
            PushLimited(summary.particles, path);
        }
        if (path.find("soundevents/") == 0 || path.find("sounds/") == 0) {
            PushLimited(summary.sounds, path);
        }
        if (path.find("panorama/") == 0 || path.find("resource/") == 0) {
            PushLimited(summary.uiAssets, path);
        }
        if (path.find("items/") != std::string::npos || path.find("/items/") != std::string::npos) {
            PushLimited(summary.itemVisuals, path);
        }

        if (EndsWith(path, ".png") || EndsWith(path, ".jpg") || EndsWith(path, ".webm")) {
            PushPreview(summary.previewAssets, {path, EndsWith(path, ".webm") ? "video-preview" : "image-preview", "Embedded preview media found in the pack."});
        } else if (path.find("portrait") != std::string::npos || path.find("icon") != std::string::npos || path.find("loadout") != std::string::npos) {
            PushPreview(summary.previewAssets, {path, "ui-preview", "Potential portrait, icon, or loadout preview asset."});
        }

        const bool semanticTarget =
            path.find("models/heroes/") != std::string::npos ||
            path.find("particles/units/heroes/") != std::string::npos ||
            path.find("materials/models/heroes/") != std::string::npos ||
            path.find("soundevents/") != std::string::npos ||
            path.find("panorama/") != std::string::npos ||
            path.find("resource/") != std::string::npos ||
            path.find("/items/") != std::string::npos;
        if (semanticTarget) {
            std::string category = "custom";
            if (path.find("models/heroes/") != std::string::npos) {
                category = "model";
            } else if (path.find("materials/") != std::string::npos) {
                category = "material";
            } else if (path.find("particles/") != std::string::npos) {
                category = "effect";
            } else if (path.find("soundevents/") != std::string::npos || path.find("sounds/") != std::string::npos) {
                category = "sound";
            } else if (path.find("panorama/") != std::string::npos || path.find("resource/") != std::string::npos) {
                category = "ui";
            } else if (path.find("/items/") != std::string::npos) {
                category = "item";
            }
            PushReplacement(summary.replacementHints, {category, path, DescribeReplacement(path)});
        }

        if (!parts.empty() && !kStandardRoots.count(root) && ContainsHeroToken(path, summary.aliases)) {
            summary.notes.push_back("Custom namespace detected: " + root);
        }
    }

    if (summary.previewAssets.empty()) {
        summary.notes.push_back("No direct PNG/JPG/WEBM preview media found in this pack. Many Dota assets are compiled *_c resources.");
    } else {
        summary.notes.push_back("Preview media detected: " + std::to_string(summary.previewAssets.size()) + " assets.");
    }
    summary.notes.push_back("Export pack built from aliases: " + JoinAliases(summary.aliases));

    std::sort(summary.notes.begin(), summary.notes.end());
    summary.notes.erase(std::unique(summary.notes.begin(), summary.notes.end()), summary.notes.end());
    return summary;
}

std::vector<VpkWriteEntry> ModAnalyzer::BuildPackEntries(const ScanSummary& summary) const {
    std::vector<VpkWriteEntry> entries;
    entries.reserve(summary.includedFiles.size() + 1);
    for (const std::string& path : summary.includedFiles) {
        const VpkEntry* entry = archive_.Find(path);
        if (!entry) {
            continue;
        }
        entries.push_back({path, archive_.ReadFile(*entry)});
    }

    std::ostringstream manifest;
    manifest << "{\n";
    manifest << "  \"hero\": \"" << JsonEscape(summary.normalizedHero) << "\",\n";
    manifest << "  \"aliases\": [";
    for (std::size_t i = 0; i < summary.aliases.size(); ++i) {
        if (i != 0) {
            manifest << ", ";
        }
        manifest << "\"" << JsonEscape(summary.aliases[i]) << "\"";
    }
    manifest << "],\n";
    manifest << "  \"seed_file_count\": " << summary.seedFiles.size() << ",\n";
    manifest << "  \"included_file_count\": " << summary.includedFiles.size() << ",\n";
    manifest << "  \"notes\": [";
    for (std::size_t i = 0; i < summary.notes.size(); ++i) {
        if (i != 0) {
            manifest << ", ";
        }
        manifest << "\"" << JsonEscape(summary.notes[i]) << "\"";
    }
    manifest << "],\n";
    manifest << "  \"files\": [\n";
    for (std::size_t i = 0; i < summary.includedFiles.size(); ++i) {
        manifest << "    \"" << JsonEscape(summary.includedFiles[i]) << "\"";
        manifest << (i + 1 == summary.includedFiles.size() ? '\n' : ',') << "";
    }
    manifest << "  ]\n";
    manifest << "}\n";

    const std::string manifestText = manifest.str();
    entries.push_back({"manifest.json", std::vector<std::uint8_t>(manifestText.begin(), manifestText.end())});
    return entries;
}

std::filesystem::path ModAnalyzer::ExportPack(const ScanSummary& summary, const std::filesystem::path& outputPathOrDirectory) const {
    std::filesystem::path outputPath = outputPathOrDirectory;
    const std::string extension = outputPath.extension().string();
    if (extension != ".vpk") {
        outputPath /= summary.normalizedHero + "_dir.vpk";
    }

    VpkWriter writer;
    writer.Write(outputPath, BuildPackEntries(summary));
    return outputPath;
}

}  // namespace dppbot
