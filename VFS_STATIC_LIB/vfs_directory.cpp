#pragma once

#include "vfs_directory.hpp"

namespace vfs {

    Directory::Directory(std::string name, Directory* parent)
        : _name(std::move(name)), _parent(parent) {
    }

    const std::string& Directory::name() const { return _name; }
    Directory* Directory::parent() const { return _parent; }

    Path Directory::full_path() const {
        if (_parent == nullptr) {
            return Path("/");
        }
        std::string path = "/";
        std::vector<const Directory*> stack;
        const Directory* current = this;
        while (current && current->_parent != nullptr) {
            stack.push_back(current);
            current = current->_parent;
        }
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
            path += (*it)->_name;
            if (it + 1 != stack.rend()) path += "/";
        }
        return Path(path);
    }

    Path Directory::relative_path() const {
        return full_path();
    }

    const std::vector<std::unique_ptr<Directory>>& Directory::subdirectories() const { return _subdirs; }
    const std::vector<std::unique_ptr<File>>& Directory::files() const { return _files; }

    Directory* Directory::add_directory(const std::string& name) {
        _subdirs.push_back(std::make_unique<Directory>(name, this));
        return _subdirs.back().get();
    }

    File* Directory::add_file(std::unique_ptr<File> file) {
        File* ptr = file.get();
        _files.push_back(std::move(file));
        return ptr;
    }

    std::optional<Directory*> Directory::find_directory(const std::string& name) {
        for (auto& d : _subdirs) {
            if (d->name() == name) return d.get();
        }
        return std::nullopt;
    }

    std::optional<File*> Directory::find_file(const std::string& name) {
        for (auto& f : _files) {
            if (f->name() == name) return f.get();
        }
        return std::nullopt;
    }

    bool Directory::remove_directory(const std::string& name) {
        for (auto it = _subdirs.begin(); it != _subdirs.end(); ++it) {
            if ((*it)->name() == name) {
                _subdirs.erase(it);
                return true;
            }
        }
        return false;
    }

    bool Directory::remove_file(const std::string& name) {
        for (auto it = _files.begin(); it != _files.end(); ++it) {
            if ((*it)->name() == name) {
                (*it)->release_resources();
                _files.erase(it);
                return true;
            }
        }
        return false;
    }

    void Directory::serialize(std::ostream& out) const {
        write_string(out, _name);
        std::uint32_t dirCount = static_cast<std::uint32_t>(_subdirs.size());
        std::uint32_t fileCount = static_cast<std::uint32_t>(_files.size());
        out.write(reinterpret_cast<const char*>(&dirCount), sizeof(dirCount));
        out.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));

        for (const auto& d : _subdirs) {
            d->serialize(out);
        }
        for (const auto& f : _files) {
            std::uint8_t fileKind =
                dynamic_cast<MemoryFile*>(f.get()) ? 0 :
                dynamic_cast<ExternalFile*>(f.get()) ? 1 :
                dynamic_cast<XObjectFile*>(f.get()) ? 2 :
                dynamic_cast<DictionaryFile*>(f.get()) ? 3 : 0;

            out.write(reinterpret_cast<const char*>(&fileKind), sizeof(fileKind));
            f->serialize(out);
        }
    }

    void Directory::deserialize(std::istream& in) {
        read_string(in, _name);
        std::uint32_t dirCount{};
        std::uint32_t fileCount{};
        in.read(reinterpret_cast<char*>(&dirCount), sizeof(dirCount));
        in.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));

        _subdirs.clear();
        _files.clear();

        for (std::uint32_t i = 0; i < dirCount; ++i) {
            auto d = std::make_unique<Directory>("", this);
            d->deserialize(in);
            _subdirs.push_back(std::move(d));
        }

        for (std::uint32_t i = 0; i < fileCount; ++i) {
            std::uint8_t fileKind{};
            in.read(reinterpret_cast<char*>(&fileKind), sizeof(fileKind));

            std::unique_ptr<File> f;
            switch (fileKind) {
            case 0: f = std::make_unique<MemoryFile>(); break;
            case 1: f = std::make_unique<ExternalFile>(); break;
            case 2: f = std::make_unique<XObjectFile>(); break;
            case 3: f = std::make_unique<DictionaryFile>(); break;
            default: f = std::make_unique<MemoryFile>(); break;
            }
            f->deserialize(in);
            _files.push_back(std::move(f));
        }
    }

} // namespace vfs
