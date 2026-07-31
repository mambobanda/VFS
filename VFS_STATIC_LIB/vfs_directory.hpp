#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <istream>
#include <ostream>

#include "vfs_file.hpp"
#include "vfs_path.hpp"

namespace vfs {

// Directory represents a node in the VFS directory tree. It owns its
// child Directory and File nodes via unique_ptr. The public API is
// intentionally small and provides operations to add/remove/find
// children, and to serialize/deserialize the subtree for persistence.
//
// Important notes:
// - add_directory/add_file return raw pointers to the stored child; the
//   Directory retains ownership.
// - remove_directory/remove_file return the removed unique_ptr so the
//   caller (e.g., VirtualFileSystem) can update caches and release
//   resources in a controlled manner.
class Directory {
public:
    Directory(std::string name = "/", Directory* parent = nullptr);

    const std::string& name() const;
    Directory* parent() const;

    Path full_path() const;

    Path relative_path() const;

    const std::vector<std::unique_ptr<Directory>>& subdirectories() const;
    const std::vector<std::unique_ptr<File>>& files() const;

    Directory* add_directory(const std::string& name);

    File* add_file(std::unique_ptr<File> file);

    std::optional<Directory*> find_directory(const std::string& name);

    std::optional<File*> find_file(const std::string& name);

    std::unique_ptr<Directory> remove_directory(const std::string& name);

    std::unique_ptr<File> remove_file(const std::string& name);

    void serialize(std::ostream& out) const;

    void deserialize(std::istream& in);

private:
    std::string _name;
    Directory* _parent{nullptr};
    std::vector<std::unique_ptr<Directory>> _subdirs;
    std::vector<std::unique_ptr<File>> _files;
};

} // namespace vfs
