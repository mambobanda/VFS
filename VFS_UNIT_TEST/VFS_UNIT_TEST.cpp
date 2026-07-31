// vfs_unit_test.cpp
// Self-contained extensive unit test for the VFS implementation.
// Build: add this file to your project or compile with the existing include paths.

#include <iostream>
#include <vector>
#include <cstddef>
#include <cstring>
#include <any>
#include <string>
#include <cstdlib>

#include "vfs_vfs.hpp"
#include "vfs_default_registry.hpp"
#include "game_player.hpp"

using namespace vfs;

static bool expect(bool cond, const char* msg) {
	if (!cond) {
		std::cerr << "FAIL: " << msg << "\n";
		return false;
	}
	std::cout << "ok: " << msg << "\n";
	return true;
}

int main() {
	try {
		register_default_types();

		VirtualFileSystem vfs;
		Path home("/home");

		// Basic tree sanity
		if (!expect(vfs.root() != nullptr, "root exists")) return 1;
		if (!expect(vfs.home() != nullptr, "home exists")) return 1;

		// 1) Memory file round-trip
		File* mem = vfs.create_memory_file(home, "notes.txt");
		if (!expect(mem != nullptr, "create_memory_file returned non-null")) return 1;
		{
			auto* mf = dynamic_cast<MemoryFile*>(mem);
			if (!expect(mf != nullptr, "memory file dynamic cast")) return 1;
			std::string text = "Hello VFS unit test";
			std::vector<std::byte> bytes(text.size());
			std::memcpy(bytes.data(), text.data(), text.size());
			mf->from_bytes(std::span<const std::byte>(bytes));
		}

		// 2) External file
		File* ext = vfs.create_external_file(home, "image.png", "C:/does/not/exist/image.png");
		expect(ext != nullptr, "create_external_file returned non-null (placeholder)");

		// 3) XObject file
		auto player = std::make_unique<game::Player>();
		player->name = "Tester";
		player->health = 314;
		File* xf = vfs.create_xobject_file(home, "player.dat", std::move(player));
		if (!expect(xf != nullptr, "create_xobject_file returned non-null")) return 1;

		// 4) Dictionary file
		File* df = vfs.create_dictionary_file(home, "settings.dict");
		if (!expect(df != nullptr, "create_dictionary_file returned non-null")) return 1;
		{
			auto* d = dynamic_cast<DictionaryFile*>(df);
			if (!expect(d != nullptr, "dictionary dynamic cast")) return 1;
			d->set("volume", 75, "int");
			d->set("username", std::string("unit_tester"), "string");
			d->set("muted", false, "bool");
		}

		// Persist VFS
		const char* fileName = "vfs_unit_test.bin";
		// remove existing
		std::remove(fileName);
		if (!expect(vfs.save_to_disk(fileName), "save_to_disk succeeded")) return 1;

		// Validate caches: fast resolve should find player before clear
		auto fast = vfs.resolve_file_fast("/home/player.dat");
		expect(fast != nullptr, "resolve_file_fast finds player before clear");

		// Remove a file and check cache updates
		bool removed = vfs.remove_file(Path("/home/notes.txt"));
		expect(removed, "remove_file succeeded for notes.txt");
		expect(vfs.resolve_file_fast("/home/notes.txt") == nullptr, "notes.txt removed from fast cache");

		// Reload from disk to exercise load code path
		vfs.clear();
		if (!expect(vfs.load_from_disk(fileName), "load_from_disk succeeded")) return 1;

		// Find and inspect XObject file
		auto fileOpt = vfs.find_file(Path("/home/player.dat"));
		if (!expect(fileOpt.has_value(), "player.dat found after load")) return 1;
		{
			auto* loadedXf = dynamic_cast<XObjectFile*>(*fileOpt);
			if (!expect(loadedXf != nullptr, "loaded file is XObjectFile")) return 1;

			// lazy init using factory
			loadedXf->lazy_initialize([] {
				return std::make_unique<game::Player>();
				});

			auto* p = dynamic_cast<game::Player*>(loadedXf->object());
			if (!expect(p != nullptr, "player object available after lazy init")) return 1;
			expect(p->name == "Tester", "player name preserved after load");
			expect(p->health == 314, "player health preserved after load");
		}

		// Find and inspect Dictionary file
		auto dictOpt = vfs.find_file(Path("/home/settings.dict"));
		if (!expect(dictOpt.has_value(), "settings.dict found after load")) return 1;
		{
			auto* df2 = dynamic_cast<DictionaryFile*>(*dictOpt);
			if (!expect(df2 != nullptr, "loaded dictionary file cast")) return 1;
			const auto& data = df2->data();
			try {
				int volume = std::any_cast<int>(data.at("volume"));
				std::string username = std::any_cast<std::string>(data.at("username"));
				bool muted = std::any_cast<bool>(data.at("muted"));
				expect(volume == 75, "dictionary 'volume' matches");
				expect(username == "unit_tester", "dictionary 'username' matches");
				expect(muted == false, "dictionary 'muted' matches");
			}
			catch (const std::exception& ex) {
				std::cerr << "Dictionary read error: " << ex.what() << "\n";
				return 1;
			}
		}

		// Corrupted load test: write a tiny invalid file and expect load to fail
		const char* badFile = "vfs_unit_test_corrupt.bin";
		{
			std::FILE* f = std::fopen(badFile, "wb");
			if (f) {
				// write garbage
				std::fwrite("BAD!", 1, 4, f);
				std::fclose(f);
			}
		}
		VirtualFileSystem vfs2;
		expect(!vfs2.load_from_disk(badFile), "loading corrupted file returns false");

		// Removal of directory (test cache rebuild)
		bool dirRemoved = vfs.remove_directory(Path("/home"));
		expect(dirRemoved, "remove_directory('/home') succeeded");
		expect(vfs.resolve_directory_fast("/home") == nullptr, "home removed from dir cache");

		// Done
		std::cout << "All checks completed.\n";
		return 0;
	}
	catch (const std::exception& ex) {
		std::cerr << "Unhandled exception in test: " << ex.what() << "\n";
		return 2;
	}
}
