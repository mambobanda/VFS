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

// Internal representation of the on-disk index for a file entry.
// This stores the file's full virtual path, a small integer
// representing which File subclass was used (fileKind), whether the
// resource is memory or external (resourceKind), and the byte offset
// and length of the serialized file body inside the VFS container.
// The index is written after the file bodies so loaders can seek
// directly to a file without fully deserializing the whole tree.
struct VfsIndexEntryInternal {
    std::string fullPath;
    std::uint8_t fileKind;
    std::uint8_t resourceKind;
    std::uint64_t offset;
    std::uint64_t length;
};

// VirtualFileSystem: in-memory representation of a simple virtual
// filesystem. The VFS exposes a Directory tree rooted at "/" and
// provides APIs to create files of several kinds, persist the whole
// tree to a single binary file, and load it back. The implementation
// uses unique_ptr ownership for Directory/File nodes, and maintains
// small string->pointer caches for fast lookup.
//
// Important responsibilities:
// - manage ownership of Directory/File objects
// - maintain caches (_dirCache/_fileCache) to speed up resolves
// - serialize/deserialize to a compact binary format with an index
// - ensure cache consistency when nodes are removed or reloaded
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

    // Remove a file or directory (updates caches)
    bool remove_file(const Path& filePath);
    bool remove_directory(const Path& dirPath);

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
