#include "vpk_writer.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace dppbot {

namespace {

constexpr std::uint32_t kVpkSignature = 0x55aa1234;
constexpr std::uint32_t kVpkVersion = 2;
constexpr std::uint16_t kDirectoryArchiveIndex = 0x7fff;
constexpr std::uint16_t kEntryTerminator = 0xffff;
constexpr std::uint32_t kOtherMd5SectionSize = 48;

struct TreeEntry {
    std::string extension;
    std::string directory;
    std::string name;
    std::uint32_t crc32 = 0;
    std::uint32_t offset = 0;
    std::uint32_t length = 0;
};

std::string NormalizePath(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    while (!value.empty() && value.front() == '/') {
        value.erase(value.begin());
    }
    return value;
}

void WritePod(std::ostream& stream, std::uint16_t value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WritePod(std::ostream& stream, std::uint32_t value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void WriteCString(std::ostream& stream, const std::string& value) {
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
    const char zero = '\0';
    stream.write(&zero, 1);
}

std::uint32_t ComputeCrc32(const std::vector<std::uint8_t>& data) {
    static std::uint32_t table[256] = {};
    static bool initialized = false;
    if (!initialized) {
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t crc = i;
            for (int bit = 0; bit < 8; ++bit) {
                crc = (crc & 1u) ? (0xedb88320u ^ (crc >> 1u)) : (crc >> 1u);
            }
            table[i] = crc;
        }
        initialized = true;
    }

    std::uint32_t crc = 0xffffffffu;
    for (std::uint8_t byte : data) {
        crc = table[(crc ^ byte) & 0xffu] ^ (crc >> 8u);
    }
    return crc ^ 0xffffffffu;
}

TreeEntry BuildTreeEntry(const VpkWriteEntry& entry, std::uint32_t offset) {
    const std::string normalized = NormalizePath(entry.path);
    const auto slash = normalized.find_last_of('/');
    const std::string filename = slash == std::string::npos ? normalized : normalized.substr(slash + 1);
    const std::string directory = slash == std::string::npos ? " " : normalized.substr(0, slash);
    const auto dot = filename.find_last_of('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 >= filename.size()) {
        throw std::runtime_error("Every exported VPK entry must have a filename extension: " + entry.path);
    }

    TreeEntry treeEntry;
    treeEntry.extension = filename.substr(dot + 1);
    treeEntry.directory = directory.empty() ? " " : directory;
    treeEntry.name = filename.substr(0, dot);
    treeEntry.crc32 = ComputeCrc32(entry.data);
    treeEntry.offset = offset;
    treeEntry.length = static_cast<std::uint32_t>(entry.data.size());
    return treeEntry;
}

}  // namespace

void VpkWriter::Write(const std::filesystem::path& outputPath, const std::vector<VpkWriteEntry>& entries) const {
    if (entries.empty()) {
        throw std::runtime_error("Cannot write an empty VPK.");
    }

    std::vector<VpkWriteEntry> sortedEntries = entries;
    std::sort(sortedEntries.begin(), sortedEntries.end(), [](const auto& left, const auto& right) {
        return NormalizePath(left.path) < NormalizePath(right.path);
    });

    std::vector<TreeEntry> treeEntries;
    treeEntries.reserve(sortedEntries.size());

    std::uint32_t currentOffset = 0;
    for (const auto& entry : sortedEntries) {
        treeEntries.push_back(BuildTreeEntry(entry, currentOffset));
        currentOffset += static_cast<std::uint32_t>(entry.data.size());
    }

    std::string tree;
    {
        std::map<std::string, std::map<std::string, std::vector<TreeEntry>>> grouped;
        for (const auto& entry : treeEntries) {
            grouped[entry.extension][entry.directory].push_back(entry);
        }

        std::ostringstream buffer;
        for (const auto& [extension, directories] : grouped) {
            WriteCString(buffer, extension);
            for (const auto& [directory, items] : directories) {
                WriteCString(buffer, directory);
                for (const auto& item : items) {
                    WriteCString(buffer, item.name);
                    WritePod(buffer, item.crc32);
                    WritePod(buffer, static_cast<std::uint16_t>(0));
                    WritePod(buffer, kDirectoryArchiveIndex);
                    WritePod(buffer, item.offset);
                    WritePod(buffer, item.length);
                    WritePod(buffer, kEntryTerminator);
                }
                WriteCString(buffer, "");
            }
            WriteCString(buffer, "");
        }
        WriteCString(buffer, "");
        tree = buffer.str();
    }

    std::error_code error;
    std::filesystem::create_directories(outputPath.parent_path(), error);
    if (error) {
        throw std::runtime_error("Failed to create output directory: " + outputPath.parent_path().string());
    }

    std::ofstream stream(outputPath, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to create VPK: " + outputPath.string());
    }

    const std::uint32_t fileDataSectionSize = currentOffset;
    WritePod(stream, kVpkSignature);
    WritePod(stream, kVpkVersion);
    WritePod(stream, static_cast<std::uint32_t>(tree.size()));
    WritePod(stream, fileDataSectionSize);
    WritePod(stream, static_cast<std::uint32_t>(0));
    WritePod(stream, kOtherMd5SectionSize);
    WritePod(stream, static_cast<std::uint32_t>(0));

    stream.write(tree.data(), static_cast<std::streamsize>(tree.size()));
    for (const auto& entry : sortedEntries) {
        stream.write(reinterpret_cast<const char*>(entry.data.data()), static_cast<std::streamsize>(entry.data.size()));
    }

    const std::array<char, kOtherMd5SectionSize> zeros = {};
    stream.write(zeros.data(), static_cast<std::streamsize>(zeros.size()));
    if (!stream) {
        throw std::runtime_error("Failed while writing VPK: " + outputPath.string());
    }
}

}  // namespace dppbot
