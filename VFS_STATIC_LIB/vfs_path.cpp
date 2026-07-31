#include "vfs_path.hpp"

namespace vfs {

    Path::Path(std::string p) : _path(std::move(p)) {}

    const std::string& Path::string() const { return _path; }

    std::vector<std::string> Path::components() const {
        std::vector<std::string> result;
        std::stringstream ss(_path);
        std::string item;
        while (std::getline(ss, item, '/')) {
            if (!item.empty())
                result.push_back(item);
        }
        return result;
    }

    std::string Path::filename() const {
        auto comps = components();
        if (comps.empty()) return {};
        return comps.back();
    }

    Path Path::parent() const {
        auto comps = components();
        if (comps.empty()) return Path("/");
        comps.pop_back();
        std::string result = "/";
        for (size_t i = 0; i < comps.size(); ++i) {
            result += comps[i];
            if (i + 1 < comps.size()) result += "/";
        }
        return Path(result);
    }

    Path Path::join(const Path& base, const std::string& name) {
        if (base.string() == "/")
            return Path("/" + name);
        return Path(base.string() + "/" + name);
    }

    std::string Path::extension() const {
        std::string fname = filename();
        auto pos = fname.rfind('.');
        if (pos == std::string::npos) return {};
        return fname.substr(pos + 1);
    }

} // namespace vfs
