#include "vpk_archive.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace dppbot {

namespace {

constexpr std::uint32_t kVpkSignature = 0x55aa1234;
constexpr std::uint16_t kDirectoryArchiveIndex = 0x7fff;

template <typename T>
T ReadPod(std::istream& stream) {
    T value{};
    stream.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!stream) {
        throw std::runtime_error("Unexpected end of VPK file.");
    }
    return value;
}

std::string ReadCString(std::istream& stream) {
    std::string value;
    char ch = '\0';
    while (stream.get(ch)) {
        if (ch == '\0') {
            return value;
        }
        value.push_back(ch);
    }
    throw std::runtime_error("Unexpected end while reading VPK string.");
}

std::string NormalizePath(std::string value) {
    std::replace(value.begin(), value.end(), '\\', '/');
    value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    std::string normalized;
    normalized.reserve(value.size());
    bool previousSlash = false;
    for (char ch : value) {
        if (ch == '/') {
            if (!previousSlash) {
                normalized.push_back('/');
            }
            previousSlash = true;
            continue;
        }
        previousSlash = false;
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

}  // namespace

void VpkArchive::Load(const std::filesystem::path& filePath) {
    filePath_ = filePath;
    entries_.clear();
    indexByPath_.clear();

    std::ifstream stream(filePath, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to open VPK: " + filePath.string());
    }

    const auto signature = ReadPod<std::uint32_t>(stream);
    version_ = ReadPod<std::uint32_t>(stream);
    if (signature != kVpkSignature) {
        throw std::runtime_error("Unsupported VPK signature in: " + filePath.string());
    }
    if (version_ != 1 && version_ != 2) {
        throw std::runtime_error("Unsupported VPK version: " + std::to_string(version_));
    }

    treeSize_ = ReadPod<std::uint32_t>(stream);
    headerSize_ = 12;
    if (version_ == 2) {
        fileDataSectionSize_ = ReadPod<std::uint32_t>(stream);
        (void)ReadPod<std::uint32_t>(stream);
        (void)ReadPod<std::uint32_t>(stream);
        (void)ReadPod<std::uint32_t>(stream);
        headerSize_ = 28;
    } else {
        fileDataSectionSize_ = 0;
    }

    while (true) {
        const std::string extension = ReadCString(stream);
        if (extension.empty()) {
            break;
        }
        while (true) {
            const std::string path = ReadCString(stream);
            if (path.empty()) {
                break;
            }
            while (true) {
                const std::string name = ReadCString(stream);
                if (name.empty()) {
                    break;
                }

                VpkEntry entry;
                std::string fullPath;
                if (path != " ") {
                    fullPath = path + "/";
                }
                fullPath += name;
                if (extension != " ") {
                    fullPath += ".";
                    fullPath += extension;
                }
                entry.path = NormalizePath(fullPath);
                entry.crc32 = ReadPod<std::uint32_t>(stream);
                entry.preloadBytes = ReadPod<std::uint16_t>(stream);
                entry.archiveIndex = ReadPod<std::uint16_t>(stream);
                entry.offset = ReadPod<std::uint32_t>(stream);
                entry.length = ReadPod<std::uint32_t>(stream);
                const auto terminator = ReadPod<std::uint16_t>(stream);
                if (terminator != 0xffff) {
                    throw std::runtime_error("Invalid VPK entry terminator in: " + filePath.string());
                }

                entry.preloadData.resize(entry.preloadBytes);
                if (!entry.preloadData.empty()) {
                    stream.read(reinterpret_cast<char*>(entry.preloadData.data()), static_cast<std::streamsize>(entry.preloadData.size()));
                    if (!stream) {
                        throw std::runtime_error("Unexpected end while reading VPK preload bytes.");
                    }
                }

                indexByPath_[entry.path] = entries_.size();
                entries_.push_back(std::move(entry));
            }
        }
    }
}

const VpkEntry* VpkArchive::Find(std::string_view normalizedPath) const {
    const auto it = indexByPath_.find(std::string(normalizedPath));
    if (it == indexByPath_.end()) {
        return nullptr;
    }
    return &entries_[it->second];
}

std::vector<std::uint8_t> VpkArchive::ReadFile(const VpkEntry& entry) const {
    std::vector<std::uint8_t> data;
    data.reserve(entry.preloadData.size() + entry.length);
    data.insert(data.end(), entry.preloadData.begin(), entry.preloadData.end());

    if (entry.length == 0) {
        return data;
    }

    std::filesystem::path archivePath;
    std::uint64_t baseOffset = 0;

    if (entry.archiveIndex == kDirectoryArchiveIndex) {
        archivePath = filePath_;
        baseOffset = static_cast<std::uint64_t>(headerSize_) + static_cast<std::uint64_t>(treeSize_);
    } else {
        std::string dirFilename = filePath_.filename().string();
        std::string baseName = dirFilename.substr(0, dirFilename.rfind("_dir.vpk"));

        std::ostringstream chunkFilename;
        chunkFilename << baseName << "_" << std::setfill('0') << std::setw(3) << entry.archiveIndex << ".vpk";

        archivePath = filePath_.parent_path() / chunkFilename.str();
        baseOffset = 0;
    }

    std::ifstream stream(archivePath, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to open VPK archive: " + archivePath.string() + " for entry: " + entry.path);
    }

    stream.seekg(static_cast<std::streamoff>(baseOffset + entry.offset), std::ios::beg);
    if (!stream) {
        throw std::runtime_error("Failed to seek to VPK entry data in: " + archivePath.string() + " for entry: " + entry.path);
    }

    const std::size_t start = data.size();
    data.resize(start + entry.length);
    stream.read(reinterpret_cast<char*>(data.data() + start), static_cast<std::streamsize>(entry.length));
    if (!stream) {
        throw std::runtime_error("Failed to read VPK entry data from: " + archivePath.string() + " for entry: " + entry.path);
    }
    return data;
}

}  // namespace dppbot
