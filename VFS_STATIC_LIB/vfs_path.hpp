#pragma once
#include <string>
#include <vector>
#include <sstream>

namespace vfs {

class Path {
public:
    Path() = default;
    explicit Path(std::string p);

    const std::string& string() const;

    std::vector<std::string> components() const;

    std::string filename() const;

    Path parent() const;

    static Path join(const Path& base, const std::string& name);

    std::string extension() const;

private:
    std::string _path;
};

} // namespace vfs

// Path is a lightweight utility for dealing with VFS-style paths
// (POSIX-like '/' separators). It's intentionally simple — it does
// not attempt to mirror std::filesystem::path but provides the small
// operations needed by the VFS (components, filename, parent, join,
// extension). Paths are treated as absolute when beginning with '/'.
