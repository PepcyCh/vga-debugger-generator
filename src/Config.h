#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

struct Group {
    std::string name;
    std::vector<std::pair<std::string, std::string>> wires;
};

struct Submodule {
    std::string name;
    std::string parent_name;
    std::vector<std::pair<std::string, std::string>> wires;
};

struct Config {
    std::string template_file;

    std::string output_dir;
    std::string mem_file;
    std::string dbg_header;

    std::string module_name;

    int header_lines = 0;
    int template_width = 80;
    int template_height = 30;

    std::unordered_map<std::string, std::string> block_prefix;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> wire_prefix;
    std::unordered_map<std::string, std::string> block_suffix;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> wire_suffix;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> wire_name;

    std::unordered_map<std::string, std::unordered_map<std::string, int>> len_bits;
    
    std::unordered_map<std::string, Submodule> submodule;

    std::unordered_map<std::string, Group> groups;

    static std::optional<Config> From(std::ifstream &fin);

    std::string FindSubmoduleOfWire(const std::string &block_name, const std::string &wire_name);
};
