#pragma once
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <cstdint>

#include "vfs_directory.hpp"
#include "vfs_file.hpp"
#include "vfs_path.hpp"

namespace vfs {

struct VfsIndexEntryInternal {
    std::string fullPath;
    std::uint8_t fileKind;
    std::uint8_t resourceKind;
    std::uint64_t offset;
    std::uint64_t length;
};

class VirtualFileSystem {
public:
    VirtualFileSystem();

    Directory* root() const;
    Directory* home() const;

    Directory* make_directory(const Path& parentPath, const std::string& name);

    File* create_memory_file(const Path& dirPath, const std::string& name);

    File* create_external_file(const Path& dirPath, const std::string& name, const std::string& externalPath);

    File* create_xobject_file(const Path& dirPath, const std::string& name, std::unique_ptr<XObject> object);

    File* create_dictionary_file(const Path& dirPath, const std::string& name);

    std::optional<Directory*> find_directory(const Path& path);

    std::optional<File*> find_file(const Path& path);

    Directory* resolve_directory_fast(const std::string& fullPath);

    File* resolve_file_fast(const std::string& fullPath);

    bool save_to_disk(const std::string& vfsFilePath) const;

    bool load_from_disk(const std::string& vfsFilePath);

    void clear();

private:
    std::unique_ptr<Directory> _root;
    Directory* _home{nullptr};

    std::unordered_map<std::string, Directory*> _dirCache;
    std::unordered_map<std::string, File*> _fileCache;
    std::vector<VfsIndexEntryInternal> _index;

    void cache_directory(const std::string& fullPath, Directory* dir);

    void cache_file(const std::string& fullPath, File* file);

    void remove_from_cache(const std::string& fullPath);

    void rebuild_cache();

    void rebuild_cache_recursive(Directory* dir, const std::string& basePath);

    std::optional<Directory*> resolve_directory(const Path& path) const;

    std::optional<File*> resolve_file(const Path& path) const;

    void serialize_with_index(std::ostream& out, Directory* dir, const std::string& basePath);
};

} // namespace vfs
