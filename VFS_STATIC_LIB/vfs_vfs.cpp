#include "vfs_vfs.hpp"

namespace vfs {

    VirtualFileSystem ::VirtualFileSystem() {
        _root = std::make_unique<Directory>("/");
        _home = _root->add_directory("home");

        cache_directory("/", _root.get());
        cache_directory("/home", _home);
    }

    Directory* VirtualFileSystem::root() const { return _root.get(); }
    Directory* VirtualFileSystem::home() const { return _home; }

    Directory* VirtualFileSystem::make_directory(const Path& parentPath, const std::string& name) {
        auto dirOpt = resolve_directory(parentPath);
        if (!dirOpt) return nullptr;
        Directory* parent = *dirOpt;
        Directory* d = parent->add_directory(name);
        std::string full = vfs::Path::join(parentPath, name).string();
        cache_directory(full, d);
        return d;
    }

    File* VirtualFileSystem::create_memory_file(const Path& dirPath, const std::string& name) {
        auto dirOpt = resolve_directory(dirPath);
        if (!dirOpt) return nullptr;
        Directory* dir = *dirOpt;
        auto mf = std::make_unique<MemoryFile>();
        mf->set_name(name);
        std::string full = vfs::Path::join(dirPath, name).string();
        mf->set_full_path(full);
        mf->set_relative_path(name);
        File* f = dir->add_file(std::move(mf));
        cache_file(full, f);
        return f;
    }

    File* VirtualFileSystem::create_external_file(const Path& dirPath, const std::string& name,
        const std::string& externalPath) {
        auto dirOpt = resolve_directory(dirPath);
        if (!dirOpt) return nullptr;
        Directory* dir = *dirOpt;
        auto ef = std::make_unique<ExternalFile>();
        ef->set_name(name);
        std::string full = vfs::Path::join(dirPath, name).string();
        ef->set_full_path(full);
        ef->set_relative_path(name);
        ef->set_external_path(externalPath);
        File* f = dir->add_file(std::move(ef));
        cache_file(full, f);
        return f;
    }

    File* VirtualFileSystem::create_xobject_file(const Path& dirPath,
        const std::string& name,
        std::unique_ptr<XObject> object) {
        auto dirOpt = resolve_directory(dirPath);
        if (!dirOpt) return nullptr;
        Directory* dir = *dirOpt;

        auto xf = std::make_unique<XObjectFile>();
        xf->set_name(name);
        std::string full = vfs::Path::join(dirPath, name).string();
        xf->set_full_path(full);
        xf->set_relative_path(name);
        xf->set_object(std::move(object));

        File* f = dir->add_file(std::move(xf));
        cache_file(full, f);
        return f;
    }

    File* VirtualFileSystem::create_dictionary_file(const Path& dirPath, const std::string& name) {
        auto dirOpt = resolve_directory(dirPath);
        if (!dirOpt) return nullptr;

        Directory* dir = *dirOpt;

        auto df = std::make_unique<DictionaryFile>();
        df->set_name(name);

        std::string full = vfs::Path::join(dirPath, name).string();
        df->set_full_path(full);
        df->set_relative_path(name);

        File* f = dir->add_file(std::move(df));
        cache_file(full, f);
        return f;
    }

    std::optional<Directory*> VirtualFileSystem::find_directory(const Path& path) {
        auto fast = resolve_directory_fast(path.string());
        if (fast) return fast;
        return resolve_directory(path);
    }

    std::optional<File*> VirtualFileSystem::find_file(const Path& path) {
        auto fast = resolve_file_fast(path.string());
        if (fast) return fast;
        return resolve_file(path);
    }

    Directory* VirtualFileSystem::resolve_directory_fast(const std::string& fullPath) {
        auto it = _dirCache.find(fullPath);
        if (it != _dirCache.end()) return it->second;
        return nullptr;
    }

    File* VirtualFileSystem::resolve_file_fast(const std::string& fullPath) {
        auto it = _fileCache.find(fullPath);
        if (it != _fileCache.end()) return it->second;
        return nullptr;
    }

    bool VirtualFileSystem::save_to_disk(const std::string& vfsFilePath) const {
        std::ofstream out(vfsFilePath, std::ios::binary);
        if (!out) return false;

        const std::uint32_t magic = 0x56465332;
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));

        std::uint64_t indexOffset = 0;
        std::uint64_t indexSize = 0;
        out.write(reinterpret_cast<const char*>(&indexOffset), sizeof(indexOffset));
        out.write(reinterpret_cast<const char*>(&indexSize), sizeof(indexSize));

        VirtualFileSystem* self = const_cast<VirtualFileSystem*>(this);
        self->_index.clear();
        self->serialize_with_index(out, _root.get(), "/");

        indexOffset = static_cast<std::uint64_t>(out.tellp());

        std::uint32_t count = static_cast<std::uint32_t>(_index.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (const auto& e : _index) {
            write_string(out, e.fullPath);
            out.write(reinterpret_cast<const char*>(&e.fileKind), sizeof(e.fileKind));
            out.write(reinterpret_cast<const char*>(&e.resourceKind), sizeof(e.resourceKind));
            out.write(reinterpret_cast<const char*>(&e.offset), sizeof(e.offset));
            out.write(reinterpret_cast<const char*>(&e.length), sizeof(e.length));
        }

        indexSize = static_cast<std::uint64_t>(out.tellp()) - indexOffset;

        out.seekp(sizeof(magic), std::ios::beg);
        out.write(reinterpret_cast<const char*>(&indexOffset), sizeof(indexOffset));
        out.write(reinterpret_cast<const char*>(&indexSize), sizeof(indexSize));

        return true;
    }

    bool VirtualFileSystem::load_from_disk(const std::string& vfsFilePath) {
        std::ifstream in(vfsFilePath, std::ios::binary);
        if (!in) return false;

        std::uint32_t magic{};
        in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x56465332) return false;

        std::uint64_t indexOffset{};
        std::uint64_t indexSize{};
        in.read(reinterpret_cast<char*>(&indexOffset), sizeof(indexOffset));
        in.read(reinterpret_cast<char*>(&indexSize), sizeof(indexSize));

        _root = std::make_unique<Directory>("/");
        _root->deserialize(in);

        _home = nullptr;
        for (auto& d : _root->subdirectories()) {
            if (d->name() == "home") {
                _home = d.get();
                break;
            }
        }
        if (!_home) {
            _home = _root->add_directory("home");
        }

        _index.clear();
        in.seekg(indexOffset, std::ios::beg);
        std::uint32_t count{};
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        _index.reserve(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            VfsIndexEntryInternal e;
            read_string(in, e.fullPath);
            in.read(reinterpret_cast<char*>(&e.fileKind), sizeof(e.fileKind));
            in.read(reinterpret_cast<char*>(&e.resourceKind), sizeof(e.resourceKind));
            in.read(reinterpret_cast<char*>(&e.offset), sizeof(e.offset));
            in.read(reinterpret_cast<char*>(&e.length), sizeof(e.length));
            _index.push_back(e);
        }

        rebuild_cache();
        return true;
    }

    void VirtualFileSystem::clear() {
        _root.reset();
        _root = std::make_unique<Directory>("/");
        _home = _root->add_directory("home");

        _dirCache.clear();
        _fileCache.clear();
        _index.clear();

        cache_directory("/", _root.get());
        cache_directory("/home", _home);
    }


    void VirtualFileSystem::cache_directory(const std::string& fullPath, Directory* dir) {
        _dirCache[fullPath] = dir;
    }

    void VirtualFileSystem::cache_file(const std::string& fullPath, File* file) {
        _fileCache[fullPath] = file;
    }

    void VirtualFileSystem::remove_from_cache(const std::string& fullPath) {
        _dirCache.erase(fullPath);
        _fileCache.erase(fullPath);
    }

    void VirtualFileSystem::rebuild_cache() {
        _dirCache.clear();
        _fileCache.clear();
        cache_directory("/", _root.get());
        rebuild_cache_recursive(_root.get(), "/");
    }

    void VirtualFileSystem::rebuild_cache_recursive(Directory* dir, const std::string& basePath) {
        for (auto& d : dir->subdirectories()) {
            std::string full = (basePath == "/") ? ("/" + d->name()) : (basePath + "/" + d->name());
            cache_directory(full, d.get());
            rebuild_cache_recursive(d.get(), full);
        }
        for (auto& f : dir->files()) {
            cache_file(f->full_path(), f.get());
        }
    }

    std::optional<Directory*> VirtualFileSystem::resolve_directory(const Path& path) const {
        if (path.string() == "/") return _root.get();
        auto comps = path.components();
        Directory* current = _root.get();
        for (auto& c : comps) {
            bool found = false;
            for (auto& d : current->subdirectories()) {
                if (d->name() == c) {
                    current = d.get();
                    found = true;
                    break;
                }
            }
            if (!found) return std::nullopt;
        }
        return current;
    }

    std::optional<File*> VirtualFileSystem::resolve_file(const Path& path) const {
        auto dirPath = path.parent();
        auto dirOpt = resolve_directory(dirPath);
        if (!dirOpt) return std::nullopt;
        Directory* dir = *dirOpt;
        std::string fname = path.filename();
        auto fOpt = dir->find_file(fname);
        if (!fOpt) return std::nullopt;
        return *fOpt;
    }

    void VirtualFileSystem::serialize_with_index(std::ostream& out, Directory* dir, const std::string& basePath) {
        write_string(out, dir->name());
        std::uint32_t dirCount = static_cast<std::uint32_t>(dir->subdirectories().size());
        std::uint32_t fileCount = static_cast<std::uint32_t>(dir->files().size());
        out.write(reinterpret_cast<const char*>(&dirCount), sizeof(dirCount));
        out.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));

        for (auto& d : dir->subdirectories()) {
            std::string childPath = (basePath == "/")
                ? ("/" + d->name())
                : (basePath + "/" + d->name());
            serialize_with_index(out, d.get(), childPath);
        }

        for (auto& f : dir->files()) {
            std::uint8_t fileKind =
                dynamic_cast<MemoryFile*>(f.get()) ? 0 :
                dynamic_cast<ExternalFile*>(f.get()) ? 1 :
                dynamic_cast<XObjectFile*>(f.get()) ? 2 :
                dynamic_cast<DictionaryFile*>(f.get()) ? 3 : 0;

            std::uint8_t resKind = static_cast<std::uint8_t>(f->resource_kind());

            std::uint64_t offset = static_cast<std::uint64_t>(out.tellp());

            out.write(reinterpret_cast<const char*>(&fileKind), sizeof(fileKind));
            f->serialize(out);

            std::uint64_t end = static_cast<std::uint64_t>(out.tellp());
            std::uint64_t length = end - offset;

            VfsIndexEntryInternal entry;
            entry.fullPath = f->full_path();
            entry.fileKind = fileKind;
            entry.resourceKind = resKind;
            entry.offset = offset;
            entry.length = length;

            _index.push_back(entry);
        }
    }


} // namespace vfs
