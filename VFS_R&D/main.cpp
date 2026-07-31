// main.cpp
#include <iostream>
#include <vector>
#include <cstddef>
#include <cstring>
#include <memory>
#include <span>
#include <string>
#include <any>

#include "vfs_vfs.hpp"
#include "vfs_default_registry.hpp"
#include "game_player.hpp"
#include "vfs_helpers.hpp"

int main() {
    using namespace vfs;

    register_default_types();

    VirtualFileSystem vfs;
    Path homePath("/home");

    // Memory file: create from string using helper
    vfs::util::create_memory_file_from_string(vfs, homePath, "notes.txt", "Hello VFS with std::byte!");

    // External file (placeholder)
    vfs.create_external_file(homePath, "image.png", "C:/real/path/image.png");

    // XObject file (create directly from object)
    auto player = std::make_unique<game::Player>();
    player->name = "Mambo";
    player->health = 150;
    vfs.create_xobject_file(homePath, "player.dat", std::move(player));

    // Dictionary file
    if (File* dictFile = vfs.create_dictionary_file(homePath, "settings.dict")) {
        if (auto* df = dynamic_cast<DictionaryFile*>(dictFile)) {
            vfs::util::set_dictionary_entry(df, "volume", 75, "int");
            vfs::util::set_dictionary_entry(df, "username", std::string("Mambo"), "string");
            vfs::util::set_dictionary_entry(df, "muted", false, "bool");
        }
    }

    // Helpers demonstrations
    // 1) trivially-copyable type -> bytes -> MemoryFile -> read back
    int myNumber = 123456;
    auto numBytes = vfs::util::to_bytes_trivial(myNumber);
    vfs::util::create_memory_file_from_bytes(vfs, homePath, "number.bin", std::span<const std::byte>(numBytes));
    if (auto f = vfs.find_file(Path("/home/number.bin"))) {
        if (auto* mf = dynamic_cast<MemoryFile*>(*f)) {
            auto stored = mf->to_bytes();
            int recovered = vfs::util::from_bytes_trivial<int>(std::span<const std::byte>(stored));
            std::cout << "number.bin contains: " << recovered << "\n";
        }
    }

    // 2) string -> bytes helpers
    vfs::util::create_memory_file_from_string(vfs, homePath, "greeting.txt", "Hello from helpers");
    if (auto f2 = vfs.find_file(Path("/home/greeting.txt"))) {
        if (auto* mf2 = dynamic_cast<MemoryFile*>(*f2)) {
            auto b = mf2->to_bytes();
            std::string s = vfs::util::bytes_to_string(std::span<const std::byte>(b));
            std::cout << "greeting.txt: " << s << "\n";
        }
    }

    // 3) write a real external file on disk and reference it
    auto extData = vfs::util::string_to_bytes("on-disk external data");
    const char* extPath = "external_sample.bin";
    vfs::util::write_file_bytes(extPath, std::span<const std::byte>(extData));
    vfs.create_external_file(homePath, "ext_sample.bin", std::string(extPath));
    // read it back directly via helper
    try {
        auto disk = vfs::util::read_file_bytes(extPath);
        std::cout << "external_sample.bin size=" << disk.size() << " contents='" << vfs::util::bytes_to_string(std::span<const std::byte>(disk)) << "'\n";
    }
    catch (const std::exception& ex) {
        std::cerr << "external file error: " << ex.what() << "\n";
    }

    // 4) XObject from bytes helper
    auto xbytes = vfs::util::string_to_bytes("simple xobject payload");
    vfs::util::create_xobject_file_from_bytes(vfs, homePath, "simple_xobj.dat", std::span<const std::byte>(xbytes));
    if (auto xfOpt = vfs.find_file(Path("/home/simple_xobj.dat"))) {
        if (auto* xf = dynamic_cast<XObjectFile*>(*xfOpt)) {
            // object should be present; inspect its byte stream
            if (xf->object()) {
                auto ob = xf->object()->getByteStream();
                std::cout << "simple_xobj.dat bytes='" << vfs::util::bytes_to_string(std::span<const std::byte>(ob)) << "'\n";
            }
        }
    }

    // 5) trivial float example
    float pi = 3.14159f;
    auto fbytes = vfs::util::to_bytes_trivial(pi);
    vfs::util::create_memory_file_from_bytes(vfs, homePath, "pi.bin", std::span<const std::byte>(fbytes));
    if (auto pfile = vfs.find_file(Path("/home/pi.bin"))) {
        if (auto* pm = dynamic_cast<MemoryFile*>(*pfile)) {
            auto vb = pm->to_bytes();
            float rf = vfs::util::from_bytes_trivial<float>(std::span<const std::byte>(vb));
            std::cout << "pi.bin -> " << rf << "\n";
        }
    }

    // Persist VFS (ensure implementation uses binary mode internally)
    if (!vfs.save_to_disk("vfs.bin")) {
        std::cerr << "Failed to save VFS to vfs.bin\n";
        return 1;
    }
    std::cout << "VFS saved.\n";

    // Clear and reload
    vfs.clear();
    if (!vfs.load_from_disk("vfs.bin")) {
        std::cerr << "Failed to load VFS from vfs.bin\n";
        return 1;
    }
    std::cout << "VFS loaded.\n";

    // Read back XObject file
    if (auto fileOpt = vfs.find_file(Path("/home/player.dat"))) {
        if (auto* xf = dynamic_cast<XObjectFile*>(*fileOpt)) {
            // If XObjectFile supports lazy init with a factory, provide a correct factory
            xf->lazy_initialize([] {
                return std::make_unique<game::Player>();
                });

            if (auto* p = dynamic_cast<game::Player*>(xf->object())) {
                std::cout << "Player: " << p->name << " health=" << p->health << "\n";
            }
            else {
                std::cerr << "Failed to retrieve Player object\n";
            }
        }
    }

    // Read back dictionary values with error handling
    if (auto dictOpt = vfs.find_file(Path("/home/settings.dict"))) {
        if (auto* df2 = dynamic_cast<DictionaryFile*>(*dictOpt)) {
            try {
                const auto& data = df2->data();
                int volume = std::any_cast<int>(data.at("volume"));
                std::string username = std::any_cast<std::string>(data.at("username"));
                bool muted = std::any_cast<bool>(data.at("muted"));
                std::cout << "Settings: volume=" << volume
                    << " username=" << username
                    << " muted=" << std::boolalpha << muted << "\n";
            }
            catch (const std::exception& ex) {
                std::cerr << "Dictionary read error: " << ex.what() << "\n";
            }
        }
    }

    return 0;
}
//#include <iostream>
//#include <vector>
//#include <cstddef>
//#include <cstring>
//#include <any>
//
//#include "vfs_vfs.hpp"
//#include "vfs_default_registry.hpp"
//#include "game_player.hpp"
//
//int main() {
//    using namespace vfs;
//
//    register_default_types();
//
//    VirtualFileSystem vfs;
//
//    Path homePath("/home");
//
//    File* memFile = vfs.create_memory_file(homePath, "notes.txt");
//    if (memFile) {
//        auto* mf = dynamic_cast<MemoryFile*>(memFile);
//        if (mf) {
//            std::string text = "Hello VFS with std::byte!";
//            std::vector<std::byte> data(text.size());
//            std::memcpy(data.data(), text.data(), text.size());
//            mf->from_bytes(std::span<const std::byte>(data));
//        }
//    }
//
//    File* extFile = vfs.create_external_file(homePath, "image.png",
//        "C:/real/path/image.png");
//
//    auto player = std::make_unique<game::Player>();
//    player->name = "Mambo";
//    player->health = 150;
//
//    File* playerFile =
//        vfs.create_xobject_file(homePath, "player.dat", std::move(player));
//
//    File* dictFile = vfs.create_dictionary_file(homePath, "settings.dict");
//    auto* df = dynamic_cast<DictionaryFile*>(dictFile);
//    if (df) {
//        df->set("volume", 75, "int");
//        df->set("username", std::string("Mambo"), "string");
//        df->set("muted", false, "bool");
//    }
//
//    if (vfs.save_to_disk("vfs.bin")) {
//        std::cout << "VFS saved.\n";
//    }
//
//    vfs.clear();
//    if (vfs.load_from_disk("vfs.bin")) {
//        std::cout << "VFS loaded.\n";
//    }
//
//    auto fileOpt = vfs.find_file(Path("/home/player.dat"));
//    if (fileOpt) {
//        auto* xf = dynamic_cast<XObjectFile*>(*fileOpt);
//        if (xf) {
//            xf->lazy_initialize([] {
//                auto p = std::make_unique<game::Player>();
//                return p;
//                });
//
//            auto* p = dynamic_cast<game::Player*>(xf->object());
//            if (p) {
//                std::cout << "Player: " << p->name
//                    << " health=" << p->health << "\n";
//            }
//        }
//    }
//
//    auto dictOpt = vfs.find_file(Path("/home/settings.dict"));
//    if (dictOpt) {
//        auto* df2 = dynamic_cast<DictionaryFile*>(*dictOpt);
//        if (df2) {
//            const auto& data = df2->data();
//            int volume = std::any_cast<int>(data.at("volume"));
//            std::string username = std::any_cast<std::string>(data.at("username"));
//            bool muted = std::any_cast<bool>(data.at("muted"));
//            std::cout << "Settings: volume=" << volume
//                << " username=" << username
//                << " muted=" << muted << "\n";
//        }
//    }
//
//    return 0;
//}
//#include <cstdint>
//#include <array>
//#include <fstream>
//#include <iostream>
//#include <string>
//#include <vector>
//#include <cassert>
//
//// Helpers: little-endian u32 write/read
//inline void write_u32_le(std::ostream& out, std::uint32_t v) {
//    unsigned char b[4] = {
//        static_cast<unsigned char>(v & 0xFFu),
//        static_cast<unsigned char>((v >> 8) & 0xFFu),
//        static_cast<unsigned char>((v >> 16) & 0xFFu),
//        static_cast<unsigned char>((v >> 24) & 0xFFu)
//    };
//    out.write(reinterpret_cast<char*>(b), 4);
//}
//
//inline std::uint32_t read_u32_le(std::istream& in) {
//    std::array<unsigned char, 4> b{};
//    in.read(reinterpret_cast<char*>(b.data()), 4);
//    if (!in) throw std::runtime_error("read_u32_le: failed to read 4 bytes");
//    return static_cast<std::uint32_t>(b[0])
//        | (static_cast<std::uint32_t>(b[1]) << 8)
//        | (static_cast<std::uint32_t>(b[2]) << 16)
//        | (static_cast<std::uint32_t>(b[3]) << 24);
//}
//
//// write/read a length-prefixed string (u32 length LE, then bytes)
//inline void write_string_le(std::ostream& out, const std::string& s) {
//    write_u32_le(out, static_cast<std::uint32_t>(s.size()));
//    if (!s.empty()) out.write(s.data(), s.size());
//}
//
//inline std::string read_string_le(std::istream& in) {
//    std::uint32_t len = read_u32_le(in);
//    if (len > static_cast<std::uint32_t>(std::numeric_limits<std::size_t>::max()))
//        throw std::runtime_error("read_string_le: length too large");
//    std::string s;
//    s.resize(static_cast<size_t>(len));
//    if (len > 0) {
//        in.read(&s[0], len);
//        if (!in) throw std::runtime_error("read_string_le: failed to read string data");
//    }
//    return s;
//}
//
//int main() {
//    const char filename[] = "roundtrip.vfs";
//
//    // Producer: write a minimal VFS archive: [magic(4)] [name:string] [dirCount:u32] [fileCount:u32]
//    {
//        std::ofstream out(filename, std::ios::binary);
//        if (!out) { std::cerr << "Failed to open for write\n"; return 1; }
//
//        // Magic + version to detect mismatches
//        const unsigned char magic[4] = { 'V', 'F', 'S', 1 };
//        out.write(reinterpret_cast<const char*>(magic), 4);
//
//        std::string name = "example_directory";
//        write_string_le(out, name);
//
//        // zero counts for simplicity
//        write_u32_le(out, 0u); // dirCount
//        write_u32_le(out, 0u); // fileCount
//    }
//
//    // Consumer: read back and validate
//    {
//        std::ifstream in(filename, std::ios::binary);
//        if (!in) { std::cerr << "Failed to open for read\n"; return 2; }
//
//        // validate magic
//        unsigned char magic_in[4];
//        in.read(reinterpret_cast<char*>(magic_in), 4);
//        if (!in) { std::cerr << "Failed to read magic\n"; return 3; }
//        if (!(magic_in[0] == 'V' && magic_in[1] == 'F' && magic_in[2] == 'S' && magic_in[3] == 1)) {
//            std::cerr << "Bad magic/version\n";
//            return 4;
//        }
//
//        std::string name = read_string_le(in);
//        std::uint32_t dirCount = read_u32_le(in);
//        std::uint32_t fileCount = read_u32_le(in);
//
//        // Basic validation
//        std::cout << "Read name: " << name << "\n";
//        std::cout << "dirCount=" << dirCount << " fileCount=" << fileCount << "\n";
//
//        // Expectation from writer
//        assert(name == "example_directory");
//        assert(dirCount == 0u);
//        assert(fileCount == 0u);
//
//        std::cout << "Round-trip succeeded\n";
//    }
//
//    return 0;
//}


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
