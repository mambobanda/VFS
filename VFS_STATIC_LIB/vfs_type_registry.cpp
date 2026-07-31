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
        // Use at() deliberately to throw if a type has not been
        // registered. Callers should ensure register_default_types()
        // has been invoked when using built-in examples.
        return serializers.at(typeName)(value);
    }

    std::any TypeRegistry::deserialize(const std::string& typeName,
        std::span<const std::byte> bytes)
    {
        // As above, using at() to signal an error if the type name is
        // unknown. This keeps logic simple: callers will catch the
        // std::out_of_range or propagate the exception as appropriate.
        return deserializers.at(typeName)(bytes);
    }

}