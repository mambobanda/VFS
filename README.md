```markdown name=README.md url=https://github.com/mambobanda/VFS/blob/master/README.md
# VFS — Virtual File System (C++)

A native C++ Virtual File System library and sample projects, focused on an in-memory/serializable VFS with support for memory files, external file proxies, XObject files, dictionary files and a small CLI + unit tests. This repository contains the core library (VFS_STATIC_LIB), unit tests, a CLI and several example/research projects.

## Features
- VirtualFileSystem abstraction with File and Directory types
- Memory-backed files and external-file proxies
- XObject files with lazy initialization (type factory support)
- Dictionary files with typed entries
- Persistence: save_to_disk / load_from_disk
- Unit tests and example usages demonstrate the public API

## Stack
- Language(s): C++ (primary)
- Build system: Visual Studio / MSBuild (solution: `VFS_R&D.slnx` / .vcxproj)
- Notable patterns: header-centric public API (vfs_*.hpp), single static library (VFS_STATIC_LIB) + test/CLI consumers

## Layout (top-level)
```
.gitattributes
.gitignore
VFS_R&D.slnx                # solution for research/sample projects
VFS_STATIC_LIB/             # core library (headers & sources)
  VFS_STATIC_LIB.vcxproj
  vfs_vfs.hpp/.cpp          # VirtualFileSystem core API & implementation
  vfs_file.hpp/.cpp         # File abstraction
  vfs_directory.hpp/.cpp    # Directory abstraction
  vfs_path.hpp/.cpp         # Path handling
  vfs_helpers.hpp           # utilities & helpers
  vfs_default_registry.*    # default type registrations / factories
  vfs_type_registry.*       # type registry
  vfs_xobject.hpp           # XObject support
VFS_UNIT_TEST/              # unit test project (vcxproj)
vfs_unit_test.cpp           # extensive standalone unit test (example usage)
vfs_cli/                    # CLI code (tool to inspect/manipulate VFS)
```

## How it fits together
VFS_STATIC_LIB exposes the main API (vfs::VirtualFileSystem and supporting types). Example consumers (unit tests, CLI, research projects) include and link against that static library to exercise saving/loading, file creation, and XObject/dictionary handling. The unit test `vfs_unit_test.cpp` contains practical usage examples you can copy into apps.

## Quick start — build & run (Windows / Visual Studio)

1. Clone:
```bash
git clone https://github.com/mambobanda/VFS.git
cd VFS
```

2. Build in Visual Studio
- Open `VFS_R&D.slnx` (or open the `VFS_STATIC_LIB` / `VFS_UNIT_TEST` projects) in Visual Studio (recommended: VS 2017/2019/2022).
- Select `Release | x64` (or `Debug | x64`) and build the solution.

3. Or build from the command line with MSBuild:
```bash
msbuild "VFS_R&D.slnx" /p:Configuration=Release /p:Platform=x64
# or build a specific project
msbuild VFS_STATIC_LIB/VFS_STATIC_LIB.vcxproj /p:Configuration=Release /p:Platform=x64
```

4. Run unit tests / example
- After building, run the unit test executable produced by the VFS unit-test project. Example possible path:
```
VFS_UNIT_TEST\x64\Release\vfs_unit_test.exe
```
- The unit test is self-contained and demonstrates the library API (creation of memory files, XObject files, saving/loading, cache checks, etc).

## Example usage (from `vfs_unit_test.cpp`)
A minimal example showing typical API calls (see the full test for more cases):

```cpp
#include "VFS_STATIC_LIB/vfs_vfs.hpp"
#include "VFS_STATIC_LIB/vfs_default_registry.hpp"

using namespace vfs;

int main() {
  register_default_types(); // registers default XObject factories, etc.

  VirtualFileSystem vfs;
  Path home("/home");

  // create memory file
  File* mem = vfs::util::create_memory_file_from_string(vfs, home, "notes.txt", "Hello VFS unit test");

  // create an XObject file using a game::Player object (example)
  auto player = std::make_unique<game::Player>();
  player->name = "Tester";
  File* xf = vfs.create_xobject_file(home, "player.dat", std::move(player));

  // save/load
  const char* fileName = "vfs_unit_test.bin";
  vfs.save_to_disk(fileName);
  vfs.clear();
  vfs.load_from_disk(fileName);
}
```

See: `vfs_unit_test.cpp` for detailed tests and API usage:
https://github.com/mambobanda/VFS/blob/master/vfs_unit_test.cpp

## Key files & references
- Core header: https://github.com/mambobanda/VFS/blob/master/VFS_STATIC_LIB/vfs_vfs.hpp
- Core implementation & headers: https://github.com/mambobanda/VFS/tree/master/VFS_STATIC_LIB
- Unit test (usage & examples): https://github.com/mambobanda/VFS/blob/master/vfs_unit_test.cpp
- Solution: https://github.com/mambobanda/VFS/blob/master/VFS_R%26D.slnx

## Notes, suggestions & next steps
- The repo is Windows/Visual-Studio centric. If cross-platform support is desired I can add:
  - CMake support (CMakeLists.txt) for Linux/macOS builds.
  - A small `examples/` folder containing sample input files used by the unit tests so users can run the demo out-of-the-box.
- Consider adding a CI workflow (GitHub Actions) to:
  - Build the library and unit-test on windows-latest.
  - Run the unit test exe as part of CI to detect regressions.
- If you want an API section in the README, I can extract and document the public API from key headers (vfs_vfs.hpp, vfs_file.hpp, vfs_path.hpp) with short code snippets.

