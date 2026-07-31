#pragma once

#include "vfs_directory.hpp"

namespace vfs {

    // Construct a Directory with an optional name and parent. The root
    // directory is represented with name "/" and parent nullptr.
    Directory::Directory(std::string name, Directory* parent)
        : _name(std::move(name)), _parent(parent) {
    }

    const std::string& Directory::name() const { return _name; }
    Directory* Directory::parent() const { return _parent; }

    // Compute the absolute path of this directory by walking up to the
    // root and joining names. Root returns "/". Note: this builds a
    // string each call; callers should cache if invoked frequently.
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

    // Create and add a child directory. The Directory keeps ownership
    // and returns a raw pointer to the new child for convenience.
    Directory* Directory::add_directory(const std::string& name) {
        _subdirs.push_back(std::make_unique<Directory>(name, this));
        return _subdirs.back().get();
    }

    // Add an owned File to this directory. The unique_ptr is moved in
    // and a raw pointer to the stored File is returned. The Directory
    // retains ownership and will destroy the File when it is removed
    // or the Directory is destroyed.
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

    // Remove a child directory by name and return ownership to the
    // caller. Returning the unique_ptr allows the VirtualFileSystem to
    // update caches, release resources, or perform other cleanup with
    // a clear ownership transfer.
    std::unique_ptr<Directory> Directory::remove_directory(const std::string& name) {
        for (auto it = _subdirs.begin(); it != _subdirs.end(); ++it) {
            if ((*it)->name() == name) {
                std::unique_ptr<Directory> removed = std::move(*it);
                _subdirs.erase(it);
                return removed;
            }
        }
        return nullptr;
    }

    // Remove a file by name and transfer ownership to the caller. The
    // caller is responsible for releasing resources (release_resources)
    // after updating caches, if necessary.
    std::unique_ptr<File> Directory::remove_file(const std::string& name) {
        for (auto it = _files.begin(); it != _files.end(); ++it) {
            if ((*it)->name() == name) {
                std::unique_ptr<File> removed = std::move(*it);
                _files.erase(it);
                return removed;
            }
        }
        return nullptr;
    }

    void Directory::serialize(std::ostream& out) const {
        write_string(out, _name);
        std::uint32_t dirCount = static_cast<std::uint32_t>(_subdirs.size());
        std::uint32_t fileCount = static_cast<std::uint32_t>(_files.size());
        write_u32_le(out, dirCount);
        write_u32_le(out, fileCount);

        for (const auto& d : _subdirs) {
            d->serialize(out);
        }
        for (const auto& f : _files) {
            std::uint8_t fileKind =
                dynamic_cast<MemoryFile*>(f.get()) ? 0 :
                dynamic_cast<ExternalFile*>(f.get()) ? 1 :
                dynamic_cast<XObjectFile*>(f.get()) ? 2 :
                dynamic_cast<DictionaryFile*>(f.get()) ? 3 : 0;

            write_u8(out, fileKind);
            f->serialize(out);
        }
    }

    void Directory::deserialize(std::istream& in) {
        read_string(in, _name);
        std::uint32_t dirCount{};
        std::uint32_t fileCount{};
        read_u32_le(in, dirCount);
        read_u32_le(in, fileCount);

        _subdirs.clear();
        _files.clear();

        for (std::uint32_t i = 0; i < dirCount; ++i) {
            auto d = std::make_unique<Directory>("", this);
            d->deserialize(in);
            _subdirs.push_back(std::move(d));
        }

        for (std::uint32_t i = 0; i < fileCount; ++i) {
            std::uint8_t fileKind{};
            read_u8(in, fileKind);

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
