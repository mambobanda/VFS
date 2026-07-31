#pragma once

#include <string>
#include <vector>
#include <memory>
#include <istream>
#include <ostream>
#include <cstddef>
#include <span>
#include <unordered_map>
#include <any>

#include "vfs_xobject.hpp"
#include "vfs_type_registry.hpp"

namespace vfs {

enum class FileType {
    Regular,
    Directory,
    Unknown
};

enum class ResourceKind {
    Memory,
    External
};

void write_string(std::ostream& out, const std::string& s);

void read_string(std::istream& in, std::string& s);

class File {
public:
    virtual ~File() = default;

    const std::string& name() const;
    void set_name(std::string n);

    FileType type() const;
    void set_type(FileType t);

    ResourceKind resource_kind() const;
    void set_resource_kind(ResourceKind k);

    void set_full_path(std::string p);
    const std::string& full_path() const;

    void set_relative_path(std::string p);
    const std::string& relative_path() const;

    virtual void serialize(std::ostream& out) const = 0;
    virtual void deserialize(std::istream& in) = 0;

    virtual std::vector<std::byte> to_bytes() const = 0;
    virtual void from_bytes(std::span<const std::byte> data) = 0;

    virtual void release_resources() = 0;

protected:
    std::string _name;
    std::string _fullPath;
    std::string _relativePath;
    FileType _type{FileType::Regular};
    ResourceKind _resourceKind{ResourceKind::Memory};
};

class MemoryFile : public File {
public:
    MemoryFile();

    void set_data(std::vector<std::byte> d);
    const std::vector<std::byte>& data() const;

    void serialize(std::ostream& out) const;

    void deserialize(std::istream& in);

    std::vector<std::byte> to_bytes() const;

    void from_bytes(std::span<const std::byte> data);

    void release_resources();

private:
    std::vector<std::byte> _data;
};

class ExternalFile : public File {
public:
    ExternalFile();

    void set_external_path(std::string p);
    const std::string& external_path() const;

    void set_external_vfs(std::string vfsPath, std::string vfsFilePath);

    const std::string& external_vfs_path() const;
    const std::string& external_vfs_file() const;

    void serialize(std::ostream& out) const;

    void deserialize(std::istream& in);

    std::vector<std::byte> to_bytes() const;

    void from_bytes(std::span<const std::byte>);

    void release_resources();

private:
    std::string _externalPath;
    std::string _externalVfsPath;
    std::string _externalVfsFile;
};

class XObjectFile : public File {
public:
    XObjectFile();

    XObjectFile(const XObjectFile& other);

    void set_object(std::unique_ptr<XObject> obj);

    XObject* object();

    void serialize(std::ostream& out) const;

    void deserialize(std::istream& in);

    std::vector<std::byte> to_bytes() const;

    void from_bytes(std::span<const std::byte> data);

    void release_resources();

    template<typename TFactory>
    void lazy_initialize(TFactory&& factory) {
        if (_object) return;
        if (_rawBytes.empty()) return;
        _object = factory();
        if (_object) {
            _object->initializeObject(std::span<const std::byte>(_rawBytes));
        }
    }

private:
    std::unique_ptr<XObject> _object;
    std::vector<std::byte> _rawBytes;
};

class DictionaryFile : public File {
public:
    DictionaryFile();

    void set(const std::string& key, std::any value, const std::string& typeName);

    const std::unordered_map<std::string, std::any>& data() const;

    void serialize(std::ostream& out) const;

    void deserialize(std::istream& in);

    std::vector<std::byte> to_bytes() const;

    void from_bytes(std::span<const std::byte>);

    void release_resources();

private:
    std::unordered_map<std::string, std::any> dict;
    std::unordered_map<std::string, std::string> types;
};

} // namespace vfs
