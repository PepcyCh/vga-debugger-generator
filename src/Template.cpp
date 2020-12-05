#include "Template.h"

#include <iostream>
#include <optional>
#include <string>
#include <algorithm>
#include <vector>

namespace {

std::string Trim(const std::string &str, char ch = ' ') {
    auto l_index = str.find_first_not_of(ch);
    if (l_index == std::string::npos) {
        l_index = 0;
    }
    auto r_index = str.find_last_not_of(ch);
    if (r_index == std::string::npos) {
        r_index = str.size() - 1;
    }
    return str.substr(l_index, r_index - l_index + 1);
}

}

std::optional<Template> Template::From(std::ifstream &fin, int header_lines, int width, int height) {
    std::vector<std::string> errors;

    Template templte {};
    std::string line;
    int max_width = 0;
    for (int i = 0; i < header_lines; i++) {
        std::getline(fin, line);
        templte.lines.emplace_back(line);
        max_width = std::max<int>(max_width, line.size());
    }

    Block curr_block {};
    curr_block.start_lineno = header_lines + 1;
    int line_count = header_lines;
    while (fin) {
        std::getline(fin, line);
        if (line.empty() || !fin) {
            break;
        }
        ++line_count;
        max_width = std::max<int>(max_width, line.size());
        if (line_count > height) {
            errors.emplace_back("Height of template file is larger than the limit " + std::to_string(height));
            break;
        }
        templte.lines.emplace_back(line);

        auto line_trimmed = Trim(line);
        if (line_trimmed.length() > 0 && line_trimmed[0] == '=' && line_trimmed.back() == '=') {
            auto block_name = Trim(Trim(line_trimmed, '='), ' ');
            if (block_name.size() > 0 && block_name.find_first_of(' ') == std::string::npos) {
                if (line_count != curr_block.start_lineno) {
                    templte.blocks.emplace_back(curr_block);
                }
                curr_block.name = block_name;
                curr_block.start_lineno = line_count;
                curr_block.wires.clear();
                continue;
            }
        }

        int p = 0, q = 0;
        std::string wire_name;
        for (int i = 0; i < line.size(); i++) {
            if (line[i] == ' ') {
                p = i + 1;
            } else if (line[i] == ':') {
                wire_name = line.substr(p, i - p);
                q = i + 1;
                while (q < line.size() && line[q] != '0') {
                    ++q;
                }
                i = q;
                while (i < line.size() && line[i] == '0') {
                    ++i;
                }
                
                int len_hex = i - q;
                if (len_hex == 0) {
                    break;
                }

                Wire wire { wire_name };
                wire.len_hex = len_hex;
                wire.temp_start_pos = q + (line_count - 1) * width;
                wire.temp_end_pos = i - 1 + (line_count - 1) * width;
                curr_block.wires.emplace_back(wire);

                p = i;
                --i;
            }
        }
    }

    if (!curr_block.wires.empty()) {
        templte.blocks.emplace_back(curr_block);
    }
    
    if (max_width > width) {
        errors.emplace_back("Width of template file is larger than the limit " + std::to_string(width));
    }
    if (!errors.empty()) {
        for (const auto &error : errors) {
            std::cerr << error << std::endl;
        }
        return std::nullopt;
    }
    return templte;
}