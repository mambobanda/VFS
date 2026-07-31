#include "vfs_file.hpp"

namespace vfs {

    inline void write_string(std::ostream& out, const std::string& s) {
        std::uint32_t len = static_cast<std::uint32_t>(s.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(s.data(), len);
    }

    inline void read_string(std::istream& in, std::string& s) {
        std::uint32_t len{};
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        s.resize(len);
        in.read(s.data(), len);
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

    void MemoryFile::serialize(std::ostream& out) const {
        write_string(out, _name);
        write_string(out, _fullPath);
        write_string(out, _relativePath);

        std::uint8_t typeVal = static_cast<std::uint8_t>(_type);
        std::uint8_t resVal = static_cast<std::uint8_t>(_resourceKind);

        out.write(reinterpret_cast<const char*>(&typeVal), sizeof(typeVal));
        out.write(reinterpret_cast<const char*>(&resVal), sizeof(resVal));

        std::uint32_t len = static_cast<std::uint32_t>(_data.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));

        if (len > 0) {
            out.write(reinterpret_cast<const char*>(_data.data()), len);
        }
    }

    void MemoryFile::deserialize(std::istream& in)  {
        read_string(in, _name);
        read_string(in, _fullPath);
        read_string(in, _relativePath);

        std::uint8_t typeVal{};
        std::uint8_t resVal{};
        in.read(reinterpret_cast<char*>(&typeVal), sizeof(typeVal));
        in.read(reinterpret_cast<char*>(&resVal), sizeof(resVal));

        _type = static_cast<FileType>(typeVal);
        _resourceKind = static_cast<ResourceKind>(resVal);

        std::uint32_t len{};
        in.read(reinterpret_cast<char*>(&len), sizeof(len));

        _data.resize(len);

        if (len > 0) {
            in.read(reinterpret_cast<char*>(_data.data()), len);
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
        write_string(out, _name);
        write_string(out, _fullPath);
        write_string(out, _relativePath);

        std::uint8_t typeVal = static_cast<std::uint8_t>(_type);
        std::uint8_t resVal = static_cast<std::uint8_t>(_resourceKind);
        out.write(reinterpret_cast<const char*>(&typeVal), sizeof(typeVal));
        out.write(reinterpret_cast<const char*>(&resVal), sizeof(resVal));

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
        in.read(reinterpret_cast<char*>(&typeVal), sizeof(typeVal));
        in.read(reinterpret_cast<char*>(&resVal), sizeof(resVal));
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
        write_string(out, _name);
        write_string(out, _fullPath);
        write_string(out, _relativePath);

        std::uint8_t typeVal = static_cast<std::uint8_t>(_type);
        std::uint8_t resVal = static_cast<std::uint8_t>(_resourceKind);

        out.write(reinterpret_cast<const char*>(&typeVal), sizeof(typeVal));
        out.write(reinterpret_cast<const char*>(&resVal), sizeof(resVal));

        std::vector<std::byte> bytes =
            _object ? _object->getByteStream() : _rawBytes;

        std::uint32_t len = static_cast<std::uint32_t>(bytes.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));

        if (len > 0) {
            out.write(reinterpret_cast<const char*>(bytes.data()), len);
        }
    }

    void XObjectFile::deserialize(std::istream& in)  {
        read_string(in, _name);
        read_string(in, _fullPath);
        read_string(in, _relativePath);

        std::uint8_t typeVal{};
        std::uint8_t resVal{};
        in.read(reinterpret_cast<char*>(&typeVal), sizeof(typeVal));
        in.read(reinterpret_cast<char*>(&resVal), sizeof(resVal));

        _type = static_cast<FileType>(typeVal);
        _resourceKind = static_cast<ResourceKind>(resVal);

        std::uint32_t len{};
        in.read(reinterpret_cast<char*>(&len), sizeof(len));

        _rawBytes.resize(len);
        if (len > 0) {
            in.read(reinterpret_cast<char*>(_rawBytes.data()), len);
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

        out.write(reinterpret_cast<const char*>(&typeVal), sizeof(typeVal));
        out.write(reinterpret_cast<const char*>(&resVal), sizeof(resVal));

        std::uint32_t count = static_cast<std::uint32_t>(dict.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));

        for (auto& [key, value] : dict) {
            write_string(out, key);
            write_string(out, types.at(key));

            auto bytes = TypeRegistry::instance().serialize(types.at(key), value);

            std::uint32_t len = static_cast<std::uint32_t>(bytes.size());
            out.write(reinterpret_cast<const char*>(&len), sizeof(len));
            if (len > 0)
                out.write(reinterpret_cast<const char*>(bytes.data()), len);
        }
    }

    void DictionaryFile::deserialize(std::istream& in)  {
        read_string(in, _name);
        read_string(in, _fullPath);
        read_string(in, _relativePath);

        std::uint8_t typeVal{};
        std::uint8_t resVal{};
        in.read(reinterpret_cast<char*>(&typeVal), sizeof(typeVal));
        in.read(reinterpret_cast<char*>(&resVal), sizeof(resVal));

        _type = static_cast<FileType>(typeVal);
        _resourceKind = static_cast<ResourceKind>(resVal);

        std::uint32_t count{};
        in.read(reinterpret_cast<char*>(&count), sizeof(count));

        dict.clear();
        types.clear();

        for (std::uint32_t i = 0; i < count; ++i) {
            std::string key;
            std::string typeName;

            read_string(in, key);
            read_string(in, typeName);

            std::uint32_t len{};
            in.read(reinterpret_cast<char*>(&len), sizeof(len));

            std::vector<std::byte> bytes(len);
            if (len > 0)
                in.read(reinterpret_cast<char*>(bytes.data()), len);

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
