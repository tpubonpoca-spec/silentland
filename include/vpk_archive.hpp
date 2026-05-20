#pragma once

#include "types.hpp"

#include <filesystem>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace dppbot {

class VpkArchive {
public:
    void Load(const std::filesystem::path& filePath);

    const std::vector<VpkEntry>& Entries() const { return entries_; }
    const std::filesystem::path& FilePath() const { return filePath_; }
    const VpkEntry* Find(std::string_view normalizedPath) const;
    std::vector<std::uint8_t> ReadFile(const VpkEntry& entry) const;

private:
    std::filesystem::path filePath_;
    std::uint32_t version_ = 0;
    std::uint32_t treeSize_ = 0;
    std::uint32_t fileDataSectionSize_ = 0;
    std::uint32_t headerSize_ = 0;
    std::vector<VpkEntry> entries_;
    std::unordered_map<std::string, std::size_t> indexByPath_;
};

}  // namespace dppbot
