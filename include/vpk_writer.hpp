#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dppbot {

struct VpkWriteEntry {
    std::string path;
    std::vector<std::uint8_t> data;
};

class VpkWriter {
public:
    void Write(const std::filesystem::path& outputPath, const std::vector<VpkWriteEntry>& entries) const;
};

}  // namespace dppbot
