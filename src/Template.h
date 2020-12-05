#pragma once

#include <fstream>
#include <string>
#include <vector>
#include <optional>

#include "Wire.h"

struct Block {
    std::string name;
    std::vector<Wire> wires;
    int start_lineno;
};

struct Template {
    std::vector<Block> blocks;
    std::vector<std::string> lines;

    static std::optional<Template> From(std::ifstream &fin, int header_lines, int width, int height);
};
