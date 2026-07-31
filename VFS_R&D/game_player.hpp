#pragma once
#include <string>
#include <vector>
#include <span>
#include <cstddef>
#include <cstring>
#include <memory>

#include "vfs_xobject.hpp"

namespace game {

    class Player : public vfs::XObject {
    public:
        std::string name;
        int health{ 100 };

        std::vector<std::byte> getByteStream() const override {
            std::vector<std::byte> bytes(sizeof(int) + name.size());
            std::memcpy(bytes.data(), &health, sizeof(int));
            std::memcpy(bytes.data() + sizeof(int), name.data(), name.size());
            return bytes;
        }

        void initializeObject(std::span<const std::byte> data) override {
            if (data.size() < sizeof(int)) {
                health = 0;
                name.clear();
                return;
            }
            std::memcpy(&health, data.data(), sizeof(int));
            size_t nameLen = data.size() - sizeof(int);
            name.assign(reinterpret_cast<const char*>(data.data() + sizeof(int)), nameLen);
        }

        std::unique_ptr<vfs::XObject> clone() const override {
            auto p = std::make_unique<Player>();
            p->name = name;
            p->health = health;
            return p;
        }
    };

} // namespace game
