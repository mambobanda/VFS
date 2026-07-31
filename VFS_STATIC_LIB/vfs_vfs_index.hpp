#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <istream>

namespace vfs {

struct VfsIndexEntry {
    std::string fullPath;
    std::uint8_t fileKind;
    std::uint8_t resourceKind;
    std::uint64_t offset;
    std::uint64_t length;
};

class VfsIndex {
public:
    bool open(const std::string& vfsFilePath);

    const std::vector<VfsIndexEntry>& all() const;

    const VfsIndexEntry* find(const std::string& fullPath) const;

    std::vector<std::byte> load_file_bytes(const VfsIndexEntry& e) const;

private:
    std::string filePath;
    std::vector<VfsIndexEntry> entries;

    static void read_string(std::istream& in, std::string& s);
};

} // namespace vfs
