#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <any>
#include <vector>
#include <span>
#include <cstddef>

namespace vfs {

// TypeRegistry holds custom serializers/deserializers for arbitrary
// value types stored inside DictionaryFile instances. This allows the
// VFS to store typed key/value pairs where the type system is driven
// by registered type names (e.g. "int", "string").
//
// Register a type with register_type(name, serializer, deserializer).
// Serializer: std::any -> bytes. Deserializer: bytes -> std::any.
class TypeRegistry {
public:
    using Serializer   = std::function<std::vector<std::byte>(const std::any&)>;
    using Deserializer = std::function<std::any(std::span<const std::byte>)>;

    static TypeRegistry& instance();

    void register_type(const std::string& name,Serializer s,Deserializer d);

    std::vector<std::byte> serialize(const std::string& typeName, const std::any& value);

    std::any deserialize(const std::string& typeName, std::span<const std::byte> bytes);

private:
    std::unordered_map<std::string, Serializer> serializers;
    std::unordered_map<std::string, Deserializer> deserializers;
};

} // namespace vfs
