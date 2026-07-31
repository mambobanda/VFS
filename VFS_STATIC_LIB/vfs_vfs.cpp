#include "vfs_vfs.hpp"
#include <functional>

namespace vfs {

    // Construct an empty VFS. We create a root directory ("/") and a
    // convenient "home" directory underneath it. The cache is primed
    // with both entries to allow fast resolves for common paths.
    VirtualFileSystem ::VirtualFileSystem() {
        _root = std::make_unique<Directory>("/");
        _home = _root->add_directory("home");

        cache_directory("/", _root.get());
        cache_directory("/home", _home);
    }

    bool VirtualFileSystem::remove_file(const Path& filePath) {
        auto dirPath = filePath.parent();
        auto dirOpt = resolve_directory(dirPath);
        if (!dirOpt) return false;
        Directory* dir = *dirOpt;
        std::string name = filePath.filename();
        auto removed = dir->remove_file(name);
        if (!removed) return false;
        // remove from caches and release resources
        remove_from_cache(removed->full_path());
        removed->release_resources();
        return true;
    }

    bool VirtualFileSystem::remove_directory(const Path& dirPath) {
        auto parentPath = dirPath.parent();
        auto dirOpt = resolve_directory(parentPath);
        if (!dirOpt) return false;
        Directory* parent = *dirOpt;
        std::string name = dirPath.filename();
        auto removed = parent->remove_directory(name);
        if (!removed) return false;

        // recursively release resources in removed tree
        std::function<void(Directory*)> releaseDir = [&](Directory* d) {
            for (auto& f : d->files()) {
                f->release_resources();
            }
            for (auto& sd : d->subdirectories()) {
                releaseDir(sd.get());
            }
        };
        releaseDir(removed.get());

        // rebuild full cache to remove any nested entries
        rebuild_cache();
        return true;
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

    // Persist the entire VFS to a single file.
    // Format (high level):
    // [magic:u32][version:u32][indexOffset:u64][indexSize:u64][directory tree + file bodies][index]
    // The implementation writes the tree and file bodies first, collects an in-memory index
    // describing where each file body was written, then appends the index and updates the
    // header (indexOffset/indexSize). Using an appended index allows fast random access later.
    bool VirtualFileSystem::save_to_disk(const std::string& vfsFilePath) const {
        try {
            std::ofstream out(vfsFilePath, std::ios::binary);
            if (!out) return false;

            // Write a magic number to quickly identify VFS files, followed
            // by a version number. The loader checks these values and
            // rejects unknown versions to avoid misinterpretation.
            const std::uint32_t magic = 0x56465332;
            const std::uint32_t version = 1;
            write_u32_le(out, magic);
            write_u32_le(out, version);

            std::uint64_t indexOffset = 0;
            std::uint64_t indexSize = 0;
            write_u64_le(out, indexOffset);
            write_u64_le(out, indexSize);

            // serialize_with_index writes the directory structure and the
            // file bodies. As it serializes files it records index
            // entries (offset and length) in _index so we can write a
            // separate index later.
            VirtualFileSystem* self = const_cast<VirtualFileSystem*>(this);
            self->_index.clear();
            self->serialize_with_index(out, _root.get(), "/");

            indexOffset = static_cast<std::uint64_t>(out.tellp());

            std::uint32_t count = static_cast<std::uint32_t>(_index.size());
            write_u32_le(out, count);

            for (const auto& e : _index) {
                write_string(out, e.fullPath);
                write_u8(out, e.fileKind);
                write_u8(out, e.resourceKind);
                write_u64_le(out, e.offset);
                write_u64_le(out, e.length);
            }

            indexSize = static_cast<std::uint64_t>(out.tellp()) - indexOffset;

            out.seekp(8, std::ios::beg); // after magic (4) + version (4)
            write_u64_le(out, indexOffset);
            write_u64_le(out, indexSize);

            return true;
        }
        catch (const std::exception&) {
            // Any error during serialization or I/O aborts and returns false.
            return false;
        }
    }

    // Load VFS from disk. This rebuilds the in-memory directory tree
    // and the in-memory caches. The loader validates the magic/version
    // and then deserializes the tree. It also reads the appended index
    // and stores it; the index can be used to implement lazy reads or
    // random access in the future.
    bool VirtualFileSystem::load_from_disk(const std::string& vfsFilePath) {

        try {
            std::ifstream in(vfsFilePath, std::ios::binary);
            if (!in) return false;


            // Validate header
            std::uint32_t magic{};
            read_u32_le(in, magic);
            if (magic != 0x56465332) return false;

            std::uint32_t version{};
            read_u32_le(in, version);
            if (version != 1) return false; // unknown file format

            std::uint64_t indexOffset{};
            std::uint64_t indexSize{};
            read_u64_le(in, indexOffset);
            read_u64_le(in, indexSize);

            // Deserialize the directory tree (recursively). The Directory
            // implementation will instantiate File subclass instances and
            // call their deserialize methods. After this step the in-memory
            // tree is reconstructed.
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

            // Read the index table. Each entry points to a file body in
            // the serialized blob which can be used for random access
            // without walking the entire tree. We store the index in
            // memory so callers may inspect it later.
            _index.clear();
            in.seekg(indexOffset, std::ios::beg);
            std::uint32_t count{};
            read_u32_le(in, count);
            _index.reserve(count);
            for (std::uint32_t i = 0; i < count; ++i) {
                VfsIndexEntryInternal e;
                read_string(in, e.fullPath);
                read_u8(in, e.fileKind);
                read_u8(in, e.resourceKind);
                read_u64_le(in, e.offset);
                read_u64_le(in, e.length);
                _index.push_back(e);
            }

            // Rebuild caches to reflect the freshly-loaded tree. This
            // populates _dirCache/_fileCache for fast resolves. If we
            // later introduce lazy loading of file bodies we must ensure
            // caches remain consistent with that behavior.
            rebuild_cache();
            return true;
        }
        catch (const std::exception&) {
            return false;
        }
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
        write_u32_le(out, dirCount);
        write_u32_le(out, fileCount);

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

            write_u8(out, fileKind);
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
