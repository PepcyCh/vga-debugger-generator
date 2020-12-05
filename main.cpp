#include <iostream>

#include "VgaDebugGenerator.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./vga_debug_generator <config-file-path>" << std::endl;
        return -1;
    }

    VgaDebugGenerator generator;
    generator.Run(argv[1]);
    
    return 0;
}
