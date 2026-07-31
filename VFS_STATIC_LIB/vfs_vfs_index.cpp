#include "vfs_vfs_index.hpp"

namespace vfs {

    bool VfsIndex::open(const std::string& vfsFilePath) {
        filePath = vfsFilePath;
        std::ifstream in(vfsFilePath, std::ios::binary);
        if (!in) return false;

        std::uint32_t magic{};
        in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x56465332) return false;

        std::uint64_t indexOffset{};
        std::uint64_t indexSize{};
        in.read(reinterpret_cast<char*>(&indexOffset), sizeof(indexOffset));
        in.read(reinterpret_cast<char*>(&indexSize), sizeof(indexSize));

        in.seekg(indexOffset, std::ios::beg);

        std::uint32_t count{};
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        entries.clear();
        entries.reserve(count);

        for (std::uint32_t i = 0; i < count; ++i) {
            VfsIndexEntry e;
            read_string(in, e.fullPath);
            in.read(reinterpret_cast<char*>(&e.fileKind), sizeof(e.fileKind));
            in.read(reinterpret_cast<char*>(&e.resourceKind), sizeof(e.resourceKind));
            in.read(reinterpret_cast<char*>(&e.offset), sizeof(e.offset));
            in.read(reinterpret_cast<char*>(&e.length), sizeof(e.length));
            entries.push_back(e);
        }

        return true;
    }

    const std::vector<VfsIndexEntry>& VfsIndex::all() const { return entries; }

    const VfsIndexEntry* VfsIndex::find(const std::string& fullPath) const {
        for (auto& e : entries) {
            if (e.fullPath == fullPath) return &e;
        }
        return nullptr;
    }

    std::vector<std::byte> VfsIndex::load_file_bytes(const VfsIndexEntry& e) const {
        std::ifstream in(filePath, std::ios::binary);
        if (!in) return {};
        in.seekg(e.offset, std::ios::beg);

        std::vector<std::byte> bytes(e.length);
        in.read(reinterpret_cast<char*>(bytes.data()), e.length);
        return bytes;
    }

    void VfsIndex::read_string(std::istream& in, std::string& s) {
        std::uint32_t len{};
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        s.resize(len);
        in.read(s.data(), len);
    }
}