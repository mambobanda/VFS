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

    bool remove_directory(const std::string& name);

    bool remove_file(const std::string& name);

    void serialize(std::ostream& out) const;

    void deserialize(std::istream& in);

private:
    std::string _name;
    Directory* _parent{nullptr};
    std::vector<std::unique_ptr<Directory>> _subdirs;
    std::vector<std::unique_ptr<File>> _files;
};

} // namespace vfs
