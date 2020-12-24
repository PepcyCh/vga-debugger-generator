#pragma once

#include <string>
#include <vector>

struct Wire {
    std::string name;
    std::string full_name;
    std::string code_name;
    std::string module_name;
    int len_hex;
    int len_bits;
    int temp_start_pos;
    int temp_end_pos;
};

struct Module {
    std::string name;
    std::string parent_name;
    std::vector<std::string> submodule_names;
    std::vector<Wire> wires;
    std::vector<Wire> wires_all;
};
