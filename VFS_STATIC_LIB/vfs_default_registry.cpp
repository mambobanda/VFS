#include "vfs_default_registry.hpp"

namespace vfs {

    void register_default_types() {
        TypeRegistry::instance().register_type(
            "int",
            [](const std::any& a) {
                int v = std::any_cast<int>(a);
                std::vector<std::byte> bytes(sizeof(int));
                std::memcpy(bytes.data(), &v, sizeof(int));
                return bytes;
            },
            [](std::span<const std::byte> bytes) {
                int v{};
                std::memcpy(&v, bytes.data(), sizeof(int));
                return std::any(v);
            }
        );

        TypeRegistry::instance().register_type(
            "float",
            [](const std::any& a) {
                float v = std::any_cast<float>(a);
                std::vector<std::byte> bytes(sizeof(float));
                std::memcpy(bytes.data(), &v, sizeof(float));
                return bytes;
            },
            [](std::span<const std::byte> bytes) {
                float v{};
                std::memcpy(&v, bytes.data(), sizeof(float));
                return std::any(v);
            }
        );

        TypeRegistry::instance().register_type(
            "string",
            [](const std::any& a) {
                const std::string& s = std::any_cast<std::string>(a);
                std::vector<std::byte> bytes(s.size());
                std::memcpy(bytes.data(), s.data(), s.size());
                return bytes;
            },
            [](std::span<const std::byte> bytes) {
                std::string s(reinterpret_cast<const char*>(bytes.data()), bytes.size());
                return std::any(s);
            }
        );

        TypeRegistry::instance().register_type(
            "bool",
            [](const std::any& a) {
                bool v = std::any_cast<bool>(a);
                std::vector<std::byte> bytes(1);
                bytes[0] = v ? std::byte{ 1 } : std::byte{ 0 };
                return bytes;
            },
            [](std::span<const std::byte> bytes) {
                bool v = (bytes.size() > 0 && bytes[0] == std::byte{ 1 });
                return std::any(v);
            }
        );

        TypeRegistry::instance().register_type(
            "null",
            [](const std::any&) {
                return std::vector<std::byte>{};
            },
            [](std::span<const std::byte>) {
                return std::any();
            }
        );
    }

} // namespace vfs
