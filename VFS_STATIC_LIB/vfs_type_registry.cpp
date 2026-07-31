#include "vfs_type_registry.hpp"

namespace vfs {

    TypeRegistry& TypeRegistry::instance() {
        static TypeRegistry reg;
        return reg;
    }

    void TypeRegistry::register_type(const std::string& name,
        Serializer s,
        Deserializer d)
    {
        serializers[name] = s;
        deserializers[name] = d;
    }

    std::vector<std::byte> TypeRegistry::serialize(const std::string& typeName,
        const std::any& value)
    {
        return serializers.at(typeName)(value);
    }

    std::any TypeRegistry::deserialize(const std::string& typeName,
        std::span<const std::byte> bytes)
    {
        return deserializers.at(typeName)(bytes);
    }

}