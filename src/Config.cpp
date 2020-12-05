#include "Config.h"

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace {

bool CheckBlockPrefix(const json &json) {
    if (!json.is_object()) {
        return false;
    }
    for (const auto &[key, value] : json.items()) {
        if (!value.is_string()) {
            return false;
        }
    }
    return true;
}

bool ParseGroup(const json &json, Config &config) {
    if (json.type() != json::value_t::array) {
        return false;
    }

    auto arr = json.get<json::array_t>();
    for (const auto &obj : arr) {
        if (!obj.is_object()) {
            return false;
        }

        if (!obj.contains("name") || !obj["name"].is_string()) {
            return false;
        }
        Group group {};
        group.name = obj["name"].get<std::string>();

        if (!obj.contains("wires") || !obj["wires"].is_object()) {
            return false;
        }
        auto wires_obj = obj["wires"];
        for (const auto &[key, value] : wires_obj.items()) {
            if (!value.is_array()) {
                return false;
            }
            auto wires_arr = value.get<json::array_t>();
            for (const auto &wire : wires_arr) {
                if (!wire.is_string()) {
                    return false;
                }
                group.wires.emplace_back(key, wire.get<std::string>());
            }
        }

        if (config.groups.count(group.name)) {
            return false;
        }
        config.groups[group.name] = group;
    }

    return true;
}

bool ParseWirePrefix(const json &json, Config &config) {
    if (!json.is_object()) {
        return false;
    }

    for (const auto &[key, value] : json.items()) {
        if (key.length() > 0 && key[0] == '*') {
            if (!value.is_string()) {
                return false;
            }

            auto group_name = key.substr(1);
            if (config.groups.count(group_name)) {
                for (const auto &wire : config.groups[group_name].wires) {
                    config.wire_prefix[wire.first][wire.second] = value.get<std::string>();
                }
            } else {
                return false;
            }
        } else {
            if (!value.is_object()) {
                return false;
            }

            for (const auto &[key2, value2] : value.items()) {
                if (!value2.is_string()) {
                    return false;
                }
                config.wire_prefix[key][key2] = value2.get<std::string>();
            }
        }
    }

    return true;
}

bool ParseWireSuffix(const json &json, Config &config) {
    if (!json.is_object()) {
        return false;
    }

    for (const auto &[key, value] : json.items()) {
        if (key.length() > 0 && key[0] == '*') {
            if (!value.is_string()) {
                return false;
            }

            auto group_name = key.substr(1);
            if (config.groups.count(group_name)) {
                for (const auto &wire : config.groups[group_name].wires) {
                    config.wire_suffix[wire.first][wire.second] = value.get<std::string>();
                }
            } else {
                return false;
            }
        } else {
            if (!value.is_object()) {
                return false;
            }

            for (const auto &[key2, value2] : value.items()) {
                if (!value2.is_string()) {
                    return false;
                }
                config.wire_suffix[key][key2] = value2.get<std::string>();
            }
        }
    }

    return true;
}

bool ParseLenBits(const json &json, Config &config) {
    if (!json.is_object()) {
        return false;
    }

    for (const auto &[key, value] : json.items()) {
        if (key.length() > 0 && key[0] == '*') {
            if (!value.is_number_integer()) {
                return false;
            }

            auto group_name = key.substr(1);
            if (config.groups.count(group_name)) {
                for (const auto &wire : config.groups[group_name].wires) {
                    config.len_bits[wire.first][wire.second] = value.get<int>();
                }
            } else {
                return false;
            }
        } else {
            if (!value.is_object()) {
                return false;
            }

            for (const auto &[key2, value2] : value.items()) {
                if (!value2.is_number_integer()) {
                    return false;
                }
                config.len_bits[key][key2] = value2.get<int>();
            }
        }
    }

    return true;
}

bool ParseSubmodule(const json &json, Config &config) {
    if (!json.is_array()) {
        return false;
    }

    auto arr = json.get<json::array_t>();
    for (const auto &obj : arr) {
        if (!obj.is_object()) {
            return false;
        }

        if (!obj.contains("name") || !obj["name"].is_string()) {
            return false;
        }
        Submodule submodule {};
        submodule.name = obj["name"].get<std::string>();

        if (!obj.contains("parent")) {
            submodule.parent_name = config.module_name;
        } else if (obj["parent"].is_string()) {
            submodule.parent_name = obj["parent"].get<std::string>();
        } else {
            return false;
        }

        if (!obj.contains("wires")) {
            return false;
        }

        auto wires_obj = obj["wires"];
        if (wires_obj.is_object()) {
            for (const auto &[key, value] : wires_obj.items()) {
                if (!value.is_array()) {
                    return false;
                }
                auto wires_arr = value.get<json::array_t>();
                for (const auto &wire : wires_arr) {
                    if (!wire.is_string()) {
                        return false;
                    }
                    submodule.wires.emplace_back(key, wire.get<std::string>());
                }
            }
        } else if (wires_obj.is_string()) {
            auto group_name = wires_obj.get<std::string>();
            if (group_name.length() > 0 && group_name[0] == '*') {
                group_name = group_name.substr(1);
                if (config.groups.count(group_name)) {
                    for (const auto &wire : config.groups[group_name].wires) {
                        submodule.wires.emplace_back(wire.first, wire.second);
                    }
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }

        if (config.submodule.count(submodule.name) || submodule.name == config.module_name) {
            return false;
        }
        config.submodule[submodule.name] = submodule;
    }

    return true;
}

}

std::optional<Config> Config::From(std::ifstream &fin) {
    json json;
    fin >> json;

    Config config {};
    std::vector<std::string> errors;

    if (json.contains("template_file")) {
        auto obj = json["template_file"];
        if (!obj.is_string()) {
            errors.emplace_back("Field 'template_file' should be a string");
        } else {
            config.template_file = obj.get<std::string>();
        }
    } else {
        errors.emplace_back("Can't find 'template_file', which is not optional");
    }

    if (json.contains("output_dir")) {
        auto obj = json["output_dir"];
        if (!obj.is_string()) {
            errors.emplace_back("Field 'output_dir' should be a string");
        } else {
            config.output_dir = obj.get<std::string>() + '/';
        }
    } else {
        config.output_dir = "./";
    }
    if (json.contains("mem_file")) {
        auto obj = json["mem_file"];
        if (!obj.is_string()) {
            errors.emplace_back("Field 'mem_file' should be a string");
        } else {
            config.mem_file = obj.get<std::string>();
        }
    } else {
        errors.emplace_back("Can't find 'mem_file', which is not optional");
    }
    if (json.contains("dbg_header")) {
        auto obj = json["dbg_header"];
        if (!obj.is_string()) {
            errors.emplace_back("Field 'dbg_header' should be a string");
        } else {
            config.dbg_header = obj.get<std::string>();
        }
    } else {
        errors.emplace_back("Can't find 'mem_file', which is not optional");
    }

    if (json.contains("module_name")) {
        auto obj = json["module_name"];
        if (!obj.is_string()) {
            errors.emplace_back("Field 'module_name' should be a string");
        } else {
            config.module_name = obj.get<std::string>();
        }
    } else {
        errors.emplace_back("Can't find 'module_name', which is not optional");
    }

    if (json.contains("header_lines")) {
        auto obj = json["header_lines"];
        if (!obj.is_number_integer()) {
            errors.emplace_back("Field 'header_lines' should be an integer");
        } else {
            config.header_lines = obj.get<int>();
        }
    }
    if (json.contains("template_width")) {
        auto obj = json["template_width"];
        if (!obj.is_number_integer()) {
            errors.emplace_back("Field 'template_width' should be an integer");
        } else {
            config.template_width = obj.get<int>();
        }
    }
    if (json.contains("template_height")) {
        auto obj = json["template_height"];
        if (!obj.is_number_integer()) {
            errors.emplace_back("Field 'template_height' should be an integer");
        } else {
            config.template_height = obj.get<int>();
        }
    }

    if (json.contains("block_prefix")) {
        auto obj = json["block_prefix"];
        if (!CheckBlockPrefix(obj)) {
            errors.emplace_back("Field 'block_prefix' has a wrong type");
        } else {
            for (const auto &[key, value] : obj.items()) {
                config.block_prefix[key] = value.get<std::string>();
            }
        }
    }
    if (json.contains("block_suffix")) {
        auto obj = json["block_suffix"];
        if (!CheckBlockPrefix(obj)) {
            errors.emplace_back("Field 'block_suffix' has a wrong type");
        } else {
            for (const auto &[key, value] : obj.items()) {
                config.block_suffix[key] = value.get<std::string>();
            }
        }
    }

    if (json.contains("wire_group")) {
        auto obj = json["wire_group"];
        if (!ParseGroup(obj, config)) {
            errors.emplace_back("Field 'wire_group' has a wrong type or some groups have the same name");
        }
    }

    if (json.contains("len_bits")) {
        auto obj = json["len_bits"];
        if (!ParseLenBits(obj, config)) {
            errors.emplace_back("Field 'len_bits' has a wrong type or wrong group reference");
        }
    }
    if (json.contains("wire_prefix")) {
        auto obj = json["wire_prefix"];
        if (!ParseWirePrefix(obj, config)) {
            errors.emplace_back("Field 'wire_prefix' has a wrong type or wrong group reference");
        }
    }
    if (json.contains("wire_suffix")) {
        auto obj = json["wire_suffix"];
        if (!ParseWireSuffix(obj, config)) {
            errors.emplace_back("Field 'wire_suffix' has a wrong type or wrong group reference");
        }
    }
    if (json.contains("submodule")) {
        auto obj = json["submodule"];
        if (!ParseSubmodule(obj, config)) {
            errors.emplace_back("Field 'submodule' has a wrong type or wrong group reference "
                "or some submodules have the same name");
        }
    }

    if (!errors.empty()) {
        for (const auto &error : errors) {
            std::cerr << error << std::endl;
        }
        return std::nullopt;
    }
    return config;
}

std::string Config::FindSubmoduleOfWire(const std::string &block_name, const std::string &wire_name) {
    for (const auto &[_, submodule] : submodule) {
        for (const auto &[block, wire] : submodule.wires) {
            if (block == block_name && wire == wire_name) {
                return submodule.name;
            }
        }
    }
    return module_name;
}
