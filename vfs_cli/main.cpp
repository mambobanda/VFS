#include <iostream>
#include <string>
#include "cli_shell.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cout << "Usage: vfs_cli <vfs.bin>\n";
        return 1;
    }

    cli::Shell shell(argv[1]);
    shell.run();

    return 0;
}
