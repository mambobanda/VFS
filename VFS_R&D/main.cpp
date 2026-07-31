#include <iostream>
#include <vector>
#include <cstddef>
#include <cstring>
#include <any>

#include "vfs_vfs.hpp"
#include "vfs_default_registry.hpp"
#include "game_player.hpp"

int main() {
    using namespace vfs;

    register_default_types();

    VirtualFileSystem vfs;

    Path homePath("/home");

    File* memFile = vfs.create_memory_file(homePath, "notes.txt");
    if (memFile) {
        auto* mf = dynamic_cast<MemoryFile*>(memFile);
        if (mf) {
            std::string text = "Hello VFS with std::byte!";
            std::vector<std::byte> data(text.size());
            std::memcpy(data.data(), text.data(), text.size());
            mf->from_bytes(std::span<const std::byte>(data));
        }
    }

    File* extFile = vfs.create_external_file(homePath, "image.png",
        "C:/real/path/image.png");

    auto player = std::make_unique<game::Player>();
    player->name = "Mambo";
    player->health = 150;

    File* playerFile =
        vfs.create_xobject_file(homePath, "player.dat", std::move(player));

    File* dictFile = vfs.create_dictionary_file(homePath, "settings.dict");
    auto* df = dynamic_cast<DictionaryFile*>(dictFile);
    if (df) {
        df->set("volume", 75, "int");
        df->set("username", std::string("Mambo"), "string");
        df->set("muted", false, "bool");
    }

    if (vfs.save_to_disk("vfs.bin")) {
        std::cout << "VFS saved.\n";
    }

    vfs.clear();
    if (vfs.load_from_disk("vfs.bin")) {
        std::cout << "VFS loaded.\n";
    }

    auto fileOpt = vfs.find_file(Path("/home/player.dat"));
    if (fileOpt) {
        auto* xf = dynamic_cast<XObjectFile*>(*fileOpt);
        if (xf) {
            xf->lazy_initialize([] {
                auto p = std::make_unique<game::Player>();
                return p;
                });

            auto* p = dynamic_cast<game::Player*>(xf->object());
            if (p) {
                std::cout << "Player: " << p->name
                    << " health=" << p->health << "\n";
            }
        }
    }

    auto dictOpt = vfs.find_file(Path("/home/settings.dict"));
    if (dictOpt) {
        auto* df2 = dynamic_cast<DictionaryFile*>(*dictOpt);
        if (df2) {
            const auto& data = df2->data();
            int volume = std::any_cast<int>(data.at("volume"));
            std::string username = std::any_cast<std::string>(data.at("username"));
            bool muted = std::any_cast<bool>(data.at("muted"));
            std::cout << "Settings: volume=" << volume
                << " username=" << username
                << " muted=" << muted << "\n";
        }
    }

    return 0;
}


//import vfs.vfs;
//import vfs.path;
//import vfs.default_registry;
//import game.player;
//
//import <iostream>;
//import <random>;
//import <string>;
//import <vector>;
//import <cstddef>;
//import <cstring>;
//
//static std::mt19937 rng(std::random_device{}());
//
//int rand_int(int min, int max) {
//    std::uniform_int_distribution<int> dist(min, max);
//    return dist(rng);
//}
//
//std::string rand_string(int len) {
//    static const char* chars = "abcdefghijklmnopqrstuvwxyz0123456789";
//    std::string s;
//    s.reserve(len);
//    for (int i = 0; i < len; ++i)
//        s.push_back(chars[rand_int(0, 35)]);
//    return s;
//}
//
//std::vector<std::byte> fake_png_bytes() {
//    // Not a real PNG, just fake binary data
//    std::string fake = "PNGFAKE" + rand_string(20);
//    std::vector<std::byte> out(fake.size());
//    std::memcpy(out.data(), fake.data(), fake.size());
//    return out;
//}
//
//std::vector<std::byte> fake_text_bytes() {
//    std::string txt = "Hello from random VFS: " + rand_string(10);
//    std::vector<std::byte> out(txt.size());
//    std::memcpy(out.data(), txt.data(), txt.size());
//    return out;
//}
//
//int main() {
//    using namespace vfs;
//
//    register_default_types();
//
//    VirtualFileSystem vfs;
//
//    // Create nested directories
//    Directory* home = vfs.home();
//    Directory* assets = vfs.make_directory(Path("/home"), "assets");
//    Directory* textures = vfs.make_directory(Path("/home/assets"), "textures");
//    Directory* misc = vfs.make_directory(Path("/home/assets"), "misc");
//
//    // Fake PNG files
//    for (int i = 0; i < 5; ++i) {
//        std::string name = "img_" + rand_string(4) + ".png";
//        File* f = vfs.create_memory_file(Path("/home/assets/textures"), name);
//        auto* mf = dynamic_cast<MemoryFile*>(f);
//        mf->from_bytes(fake_png_bytes());
//    }
//
//    // Fake text files
//    for (int i = 0; i < 3; ++i) {
//        std::string name = "note_" + rand_string(4) + ".txt";
//        File* f = vfs.create_memory_file(Path("/home/assets/misc"), name);
//        auto* mf = dynamic_cast<MemoryFile*>(f);
//        mf->from_bytes(fake_text_bytes());
//    }
//
//    // Dictionary file with random values
//    File* dictFile = vfs.create_dictionary_file(Path("/home"), "settings.dict");
//    auto* df = dynamic_cast<DictionaryFile*>(dictFile);
//    df->set("volume", rand_int(0, 100), "int");
//    df->set("username", std::string("user_" + rand_string(5)), "string");
//    df->set("muted", rand_int(0, 1) == 1, "bool");
//
//    // XObjectFile with random Player
//    auto player = std::make_unique<game::Player>();
//    player->name = "Player_" + rand_string(4);
//    player->health = rand_int(50, 200);
//
//    vfs.create_xobject_file(Path("/home"), "player.dat", std::move(player));
//
//    // ExternalFile referencing another VFS
//    File* ext = vfs.create_external_file(Path("/home"), "linked.vfs",
//        "C:/fake/path/other_vfs.bin");
//    auto* ef = dynamic_cast<ExternalFile*>(ext);
//    ef->set_external_vfs("other_vfs.bin", "/home/player.dat");
//
//    // Save final VFS
//    if (vfs.save_to_disk("C:/test/Aida/VFS/data/random_vfs.bin")) {
//        std::cout << "Generated random_vfs.bin\n";
//    }
//    else {
//        std::cout << "Failed to save VFS\n";
//    }
//
//    return 0;
//}
//
