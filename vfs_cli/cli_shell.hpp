#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include "cli_util.hpp"
#include "vfs_vfs_index.hpp"
#include "vfs_path.hpp"

namespace cli {

class Shell {
public:
    Shell(const std::string& vfsPath)
        : _vfsPath(vfsPath)
    {
        if (!_index.open(vfsPath)) {
            std::cout << "Failed to open VFS file.\n";
            _loaded = false;
        } else {
            _loaded = true;
        }
    }

    void run() {
        if (!_loaded) return;

        std::cout << "VFS CLI ready. Type 'help'.\n";

        while (true) {
            std::cout << _cwd << "> ";
            std::string line;
            std::getline(std::cin, line);

            auto args = cli::split(line);
            if (args.empty()) continue;

            const std::string& cmd = args[0];

            if (cmd == "exit") break;
            else if (cmd == "help") help();
            else if (cmd == "pwd") pwd();
            else if (cmd == "ls") ls();
            else if (cmd == "cd") {
                if (args.size() < 2) std::cout << "cd <path>\n";
                else cd(args[1]);
            }
            else if (cmd == "info") {
                if (args.size() < 2) std::cout << "info <file>\n";
                else info(args[1]);
            }
            else if (cmd == "cat") {
                if (args.size() < 2) std::cout << "cat <file>\n";
                else cat(args[1]);
            }
            else {
                std::cout << "Unknown command. Type 'help'.\n";
            }
        }
    }

private:
    bool _loaded{false};
    std::string _vfsPath;
    vfs::VfsIndex _index;
    std::string _cwd = "/";

    void help() {
        std::cout << "Commands:\n"
                  << "  ls               - list files in current directory\n"
                  << "  cd <path>        - change directory\n"
                  << "  pwd              - print current directory\n"
                  << "  info <file>      - show metadata\n"
                  << "  cat <file>       - dump raw bytes\n"
                  << "  exit             - quit\n"
                  << "  help             - show this help\n";
    }

    void pwd() {
        std::cout << _cwd << "\n";
    }

    void ls() {

        /*std::string prefix = (_cwd == "/") ? "/" : _cwd + "/";

        std::vector<std::string> items;

        for (auto& e : _index.all()) {
            if (!cli::starts_with(e.fullPath, prefix))
                continue;

            std::string rest = e.fullPath.substr(prefix.size());

            if (rest.find('/') != std::string::npos)
                continue;

            items.push_back(rest);
        }

        std::sort(items.begin(), items.end());
        for (auto& i : items)
            std::cout << i << "\n";*/

        std::string prefix = (_cwd == "/") ? "/" : _cwd + "/";

        std::vector<std::string> dirs;
        std::vector<std::string> files;

        for (auto& e : _index.all()) {
            if (!cli::starts_with(e.fullPath, prefix))
                continue;

            std::string rest = e.fullPath.substr(prefix.size());
            if (rest.empty())
                continue;

            auto slashPos = rest.find('/');

            if (slashPos == std::string::npos) {
                // Direct child file
                files.push_back(rest);
            }
            else {
                // Direct child directory name is before first '/'
                std::string dirName = rest.substr(0, slashPos);
                if (std::find(dirs.begin(), dirs.end(), dirName) == dirs.end())
                    dirs.push_back(dirName);
            }
        }

        std::sort(dirs.begin(), dirs.end());
        std::sort(files.begin(), files.end());

        for (auto& d : dirs)
            std::cout << "[dir] " << d << "\n";
        for (auto& f : files)
            std::cout << f << "\n";
    }

    void cd(const std::string& path) {
        if (path == "/") {
            _cwd = "/";
            return;
        }

        vfs::Path newPath = vfs::Path::join(vfs::Path(_cwd), path);

        bool exists = false;
        std::string prefix = newPath.string() + "/";

        for (auto& e : _index.all()) {
            if (cli::starts_with(e.fullPath, prefix)) {
                exists = true;
                break;
            }
        }

        if (exists) {
            _cwd = newPath.string();
        } else {
            std::cout << "Directory not found.\n";
        }
    }

    void info(const std::string& file) {
        vfs::Path p = vfs::Path::join(vfs::Path(_cwd), file);
        auto* entry = _index.find(p.string());
        if (!entry) {
            std::cout << "File not found.\n";
            return;
        }

        std::cout << "File: " << entry->fullPath << "\n"
                  << "Kind: " << int(entry->fileKind) << "\n"
                  << "Resource: " << int(entry->resourceKind) << "\n"
                  << "Offset: " << entry->offset << "\n"
                  << "Length: " << entry->length << "\n";
    }

    void cat(const std::string& file) {
        vfs::Path p = vfs::Path::join(vfs::Path(_cwd), file);
        auto* entry = _index.find(p.string());
        if (!entry) {
            std::cout << "File not found.\n";
            return;
        }

        auto bytes = _index.load_file_bytes(*entry);

        std::cout << "Raw bytes (" << bytes.size() << "):\n";
        for (auto b : bytes) {
            std::cout << std::to_integer<int>(b) << " ";
        }
        std::cout << "\n";
    }
};

} // namespace cli
