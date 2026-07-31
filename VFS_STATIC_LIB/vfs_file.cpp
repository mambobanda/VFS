#include "vfs_file.hpp"
#include <stdexcept>

namespace vfs {

    // ---- Binary I/O helpers -------------------------------------------------
    // The VFS uses a compact binary format. These helpers centralize
    // little-endian integer encoding and checked reads/writes. They
    // throw std::runtime_error on failure which allows higher-level
    // functions to abort and return an error result instead of
    // continuing with partially-read data.

    void write_u8(std::ostream& out, std::uint8_t v) {
        out.put(static_cast<char>(v));
        if (!out) throw std::runtime_error("I/O write error (u8)");
    }

    void read_u8(std::istream& in, std::uint8_t& v) {
        int ch = in.get();
        if (ch == EOF) throw std::runtime_error("I/O read error (u8)");
        v = static_cast<std::uint8_t>(ch);
    }

    void write_u32_le(std::ostream& out, std::uint32_t v) {
        unsigned char b[4] = {
            static_cast<unsigned char>(v & 0xFFu),
            static_cast<unsigned char>((v >> 8) & 0xFFu),
            static_cast<unsigned char>((v >> 16) & 0xFFu),
            static_cast<unsigned char>((v >> 24) & 0xFFu)
        };
        out.write(reinterpret_cast<const char*>(b), 4);
        if (!out) throw std::runtime_error("I/O write error (u32)");
    }

    void read_u32_le(std::istream& in, std::uint32_t& v) {
        unsigned char b[4];
        in.read(reinterpret_cast<char*>(b), 4);
        if (!in) throw std::runtime_error("I/O read error (u32)");
        v = static_cast<std::uint32_t>(b[0]) |
            (static_cast<std::uint32_t>(b[1]) << 8) |
            (static_cast<std::uint32_t>(b[2]) << 16) |
            (static_cast<std::uint32_t>(b[3]) << 24);
    }

    void write_u64_le(std::ostream& out, std::uint64_t v) {
        unsigned char b[8];
        for (int i = 0; i < 8; ++i) b[i] = static_cast<unsigned char>((v >> (8 * i)) & 0xFFu);
        out.write(reinterpret_cast<const char*>(b), 8);
        if (!out) throw std::runtime_error("I/O write error (u64)");
    }

    void read_u64_le(std::istream& in, std::uint64_t& v) {
        unsigned char b[8];
        in.read(reinterpret_cast<char*>(b), 8);
        if (!in) throw std::runtime_error("I/O read error (u64)");
        v = 0;
        for (int i = 0; i < 8; ++i) v |= (static_cast<std::uint64_t>(b[i]) << (8 * i));
    }

    // Strings are encoded as a 32-bit little-endian length followed by
    // raw bytes. We validate I/O after each operation and throw on
    // failure to allow callers to abort cleanly.
    void write_string(std::ostream& out, const std::string& s) {
        std::uint32_t len = static_cast<std::uint32_t>(s.size());
        write_u32_le(out, len);
        if (len > 0) {
            out.write(s.data(), len);
            if (!out) throw std::runtime_error("I/O write error (string data)");
        }
    }

    void read_string(std::istream& in, std::string& s) {
        std::uint32_t len{};
        read_u32_le(in, len);
        s.resize(len);
        if (len > 0) {
            in.read(s.data(), len);
            if (!in) throw std::runtime_error("I/O read error (string data)");
        }
    }


    const std::string& File::name() const { return _name; }
    void File::set_name(std::string n) { _name = std::move(n); }

    FileType File::type() const { return _type; }
    void File::set_type(FileType t) { _type = t; }

    ResourceKind File::resource_kind() const { return _resourceKind; }
    void File::set_resource_kind(ResourceKind k) { _resourceKind = k; }

    void File::set_full_path(std::string p) { _fullPath = std::move(p); }
    const std::string& File::full_path() const { return _fullPath; }

    void File::set_relative_path(std::string p) { _relativePath = std::move(p); }
    const std::string& File::relative_path() const { return _relativePath; }

    MemoryFile::MemoryFile() {
        set_resource_kind(ResourceKind::Memory);
        set_type(FileType::Regular);
    }

    void MemoryFile::set_data(std::vector<std::byte> d) { _data = std::move(d); }
    const std::vector<std::byte>& MemoryFile::data() const { return _data; }

    // MemoryFile serialization
    // Format: [name][fullPath][relativePath][type:u8][resKind:u8][dataLen:u32][data]
    // The write helpers validate I/O and throw on failure.
    void MemoryFile::serialize(std::ostream& out) const {
        write_string(out, _name);
        write_string(out, _fullPath);
        write_string(out, _relativePath);

        std::uint8_t typeVal = static_cast<std::uint8_t>(_type);
        std::uint8_t resVal = static_cast<std::uint8_t>(_resourceKind);

        write_u8(out, typeVal);
        write_u8(out, resVal);

        std::uint32_t len = static_cast<std::uint32_t>(_data.size());
        write_u32_le(out, len);

        if (len > 0) {
            out.write(reinterpret_cast<const char*>(_data.data()), len);
            if (!out) throw std::runtime_error("I/O write error (memory data)");
        }
    }

    void MemoryFile::deserialize(std::istream& in)  {
        read_string(in, _name);
        read_string(in, _fullPath);
        read_string(in, _relativePath);

        std::uint8_t typeVal{};
        std::uint8_t resVal{};
        read_u8(in, typeVal);
        read_u8(in, resVal);

        _type = static_cast<FileType>(typeVal);
        _resourceKind = static_cast<ResourceKind>(resVal);

        std::uint32_t len{};
        read_u32_le(in, len);

        _data.resize(len);

        if (len > 0) {
            in.read(reinterpret_cast<char*>(_data.data()), len);
            if (!in) throw std::runtime_error("I/O read error (memory data)");
        }
    }

    std::vector<std::byte> MemoryFile::to_bytes() const  {
        return _data;
    }

    void MemoryFile::from_bytes(std::span<const std::byte> data)  {
        _data.assign(data.begin(), data.end());
    }

    void MemoryFile::release_resources()  {
        _data.clear();
        _data.shrink_to_fit();
    }


    ExternalFile::ExternalFile() {
        set_resource_kind(ResourceKind::External);
        set_type(FileType::Regular);
    }

    void ExternalFile::set_external_path(std::string p) { _externalPath = std::move(p); }
    const std::string& ExternalFile::external_path() const { return _externalPath; }

    void ExternalFile::set_external_vfs(std::string vfsPath, std::string vfsFilePath) {
        _externalVfsPath = std::move(vfsPath);
        _externalVfsFile = std::move(vfsFilePath);
    }

    const std::string& ExternalFile::external_vfs_path() const { return _externalVfsPath; }
    const std::string& ExternalFile::external_vfs_file() const { return _externalVfsFile; }

    void ExternalFile::serialize(std::ostream& out) const  {
        // ExternalFile serialization stores the path information that
        // points to the host filesystem resource. External files are
        // not embedded in the VFS; instead the VFS keeps a reference
        // so callers can resolve where to find the real data.
        write_string(out, _name);
        write_string(out, _fullPath);
        write_string(out, _relativePath);

        std::uint8_t typeVal = static_cast<std::uint8_t>(_type);
        std::uint8_t resVal = static_cast<std::uint8_t>(_resourceKind);
        write_u8(out, typeVal);
        write_u8(out, resVal);

        write_string(out, _externalPath);
        write_string(out, _externalVfsPath);
        write_string(out, _externalVfsFile);
    }

    void ExternalFile::deserialize(std::istream& in)  {
        read_string(in, _name);
        read_string(in, _fullPath);
        read_string(in, _relativePath);

        std::uint8_t typeVal{};
        std::uint8_t resVal{};
        read_u8(in, typeVal);
        read_u8(in, resVal);
        _type = static_cast<FileType>(typeVal);
        _resourceKind = static_cast<ResourceKind>(resVal);

        read_string(in, _externalPath);
        read_string(in, _externalVfsPath);
        read_string(in, _externalVfsFile);
    }

    std::vector<std::byte> ExternalFile::to_bytes() const  {
        return {};
    }

    void ExternalFile::from_bytes(std::span<const std::byte>)  {
    }

    void ExternalFile::release_resources()  {
    }


    XObjectFile::XObjectFile() {
        set_resource_kind(ResourceKind::Memory);
        set_type(FileType::Regular);
    }

    XObjectFile::XObjectFile(const XObjectFile& other) {
        _name = other._name;
        _fullPath = other._fullPath;
        _relativePath = other._relativePath;
        _type = other._type;
        _resourceKind = other._resourceKind;
        _rawBytes = other._rawBytes;
        if (other._object) {
            _object = other._object->clone();
        }
    }

    void XObjectFile::set_object(std::unique_ptr<XObject> obj) {
        _object = std::move(obj);
    }

    XObject* XObjectFile::object() {
        return _object.get();
    }

    void XObjectFile::serialize(std::ostream& out) const {
        // XObjectFile stores a serialized blob representing an XObject.
        // If an in-memory object exists we ask it for its byte stream,
        // otherwise we write the raw bytes that were previously stored.
        write_string(out, _name);
        write_string(out, _fullPath);
        write_string(out, _relativePath);

        std::uint8_t typeVal = static_cast<std::uint8_t>(_type);
        std::uint8_t resVal = static_cast<std::uint8_t>(_resourceKind);

        // Use the checked helpers so failures throw consistently.
        write_u8(out, typeVal);
        write_u8(out, resVal);

        std::vector<std::byte> bytes =
            _object ? _object->getByteStream() : _rawBytes;

        std::uint32_t len = static_cast<std::uint32_t>(bytes.size());
        write_u32_le(out, len);

        if (len > 0) {
            out.write(reinterpret_cast<const char*>(bytes.data()), len);
            if (!out) throw std::runtime_error("I/O write error (xobject bytes)");
        }
    }

    void XObjectFile::deserialize(std::istream& in)  {
        // Read metadata and the serialized byte blob. Use the checked helpers
        // for consistent error reporting on read failures.
        read_string(in, _name);
        read_string(in, _fullPath);
        read_string(in, _relativePath);

        std::uint8_t typeVal{};
        std::uint8_t resVal{};
        read_u8(in, typeVal);
        read_u8(in, resVal);

        _type = static_cast<FileType>(typeVal);
        _resourceKind = static_cast<ResourceKind>(resVal);

        std::uint32_t len{};
        read_u32_le(in, len);

        _rawBytes.resize(len);
        if (len > 0) {
            in.read(reinterpret_cast<char*>(_rawBytes.data()), len);
            if (!in) throw std::runtime_error("I/O read error (xobject bytes)");
        }
    }

    std::vector<std::byte> XObjectFile::to_bytes() const  {
        if (_object) return _object->getByteStream();
        return _rawBytes;
    }

    void XObjectFile::from_bytes(std::span<const std::byte> data)  {
        _rawBytes.assign(data.begin(), data.end());
        if (_object) {
            _object->initializeObject(data);
        }
    }

    void XObjectFile::release_resources()  {
        _object.reset();
        _rawBytes.clear();
        _rawBytes.shrink_to_fit();
    }


    DictionaryFile::DictionaryFile() {
        set_resource_kind(ResourceKind::Memory);
        set_type(FileType::Regular);
    }

    void DictionaryFile::set(const std::string& key, std::any value, const std::string& typeName) {
        dict[key] = value;
        types[key] = typeName;
    }

    const std::unordered_map<std::string, std::any>& DictionaryFile::data() const {
        return dict;
    }

    void DictionaryFile::serialize(std::ostream& out) const {
        write_string(out, _name);
        write_string(out, _fullPath);
        write_string(out, _relativePath);

        std::uint8_t typeVal = static_cast<std::uint8_t>(_type);
        std::uint8_t resVal = static_cast<std::uint8_t>(_resourceKind);

        write_u8(out, typeVal);
        write_u8(out, resVal);

        std::uint32_t count = static_cast<std::uint32_t>(dict.size());
        write_u32_le(out, count);

        for (auto& [key, value] : dict) {
            write_string(out, key);
            write_string(out, types.at(key));

            auto bytes = TypeRegistry::instance().serialize(types.at(key), value);

            std::uint32_t len = static_cast<std::uint32_t>(bytes.size());
            write_u32_le(out, len);
            if (len > 0) {
                out.write(reinterpret_cast<const char*>(bytes.data()), len);
                if (!out) throw std::runtime_error("I/O write error (dict value bytes)");
            }
        }
    }

    void DictionaryFile::deserialize(std::istream& in)  {
        read_string(in, _name);
        read_string(in, _fullPath);
        read_string(in, _relativePath);

        std::uint8_t typeVal{};
        std::uint8_t resVal{};
        read_u8(in, typeVal);
        read_u8(in, resVal);

        _type = static_cast<FileType>(typeVal);
        _resourceKind = static_cast<ResourceKind>(resVal);

        std::uint32_t count{};
        read_u32_le(in, count);

        dict.clear();
        types.clear();

        for (std::uint32_t i = 0; i < count; ++i) {
            std::string key;
            std::string typeName;

            read_string(in, key);
            read_string(in, typeName);

            std::uint32_t len{};
            read_u32_le(in, len);

            std::vector<std::byte> bytes(len);
            if (len > 0) {
                in.read(reinterpret_cast<char*>(bytes.data()), len);
                if (!in) throw std::runtime_error("I/O read error (dict value bytes)");
            }

            std::any value = TypeRegistry::instance().deserialize(
                typeName, std::span<const std::byte>(bytes));

            dict[key] = value;
            types[key] = typeName;
        }
    }

    std::vector<std::byte> DictionaryFile::to_bytes() const {
        return {};
    }

    void DictionaryFile::from_bytes(std::span<const std::byte>) {
    }

    void DictionaryFile::release_resources() {
        dict.clear();
        types.clear();
    }


} // namespace vfs
