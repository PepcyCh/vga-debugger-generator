#include "VgaDebugGenerator.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>

#include "Config.h"
#include "Template.h"
#include "Wire.h"

void VgaDebugGenerator::Run(const std::string &config_file) {
    try {
        LoadConfig(config_file);
        LoadTemplate();
        ProcessConfig();
        ProcessModules(config.module_name);
        Generate();
    } catch (const std::string &error_msg) {
        std::cerr << error_msg << std::endl;
    }
}

void VgaDebugGenerator::LoadConfig(const std::string &config_file) {
    std::ifstream config_fin(config_file);
    if (!config_fin) {
        throw "Failed to open config file '" + config_file + "'";
    }
    auto config_opt = Config::From(config_fin);
    if (!config_opt.has_value()) {
        throw std::string("Failed to parse config file due to above reasons");
    }
    config = config_opt.value();
}

void VgaDebugGenerator::LoadTemplate() {
    std::ifstream temp_fin(config.template_file);
    if (!temp_fin) {
        throw "Failed to open template file '" + config.template_file + "'";
    }
    auto temp_opt = Template::From(temp_fin, config.header_lines, config.template_width, config.template_height);
    if (!temp_opt.has_value()) {
        throw std::string("Failed to parse template file due to above reasons");
    }
    templte = temp_opt.value();
}

void VgaDebugGenerator::ProcessConfig() {
    modules[config.module_name] = Module { config.module_name, "" };
    for (const auto &[_, submodule] : config.submodule) {
        if (submodule.parent_name == config.module_name || config.submodule.count(submodule.parent_name)) {
            modules[submodule.name] = Module { submodule.name, submodule.parent_name };
        } else {
            throw "Can't find parent module '" + submodule.parent_name + "' of module '" + submodule.name + "'";
        }
    }
    for (const auto &[_, submodule] : config.submodule) {
        modules[submodule.parent_name].submodule_names.emplace_back(submodule.name);
    }

    for (auto &block : templte.blocks) {
        std::string block_prefix = "";
        if (config.block_prefix.count(block.name)) {
            block_prefix = config.block_prefix[block.name];
        }
        std::string block_suffix = "";
        if (config.block_suffix.count(block.name)) {
            block_suffix = config.block_suffix[block.name];
        }

        bool len_bits_block_flag = config.len_bits.count(block.name);
        bool wire_prefix_block_flag = config.wire_prefix.count(block.name);
        bool wire_suffix_block_flag = config.wire_suffix.count(block.name);

        for (auto &wire : block.wires) {
            if (len_bits_block_flag && config.len_bits[block.name].count(wire.name)) {
                wire.len_bits = config.len_bits[block.name][wire.name];
            } else {
                wire.len_bits = wire.len_hex == 1 ? 1 : wire.len_hex * 4;
            }

            // prefix
            if (wire_prefix_block_flag && config.wire_prefix[block.name].count(wire.name)) {
                wire.full_name = config.wire_prefix[block.name][wire.name] + wire.name;
            } else {
                wire.full_name = block_prefix + wire.name;
            }
            // suffix
            if (wire_suffix_block_flag && config.wire_suffix[block.name].count(wire.name)) {
                wire.full_name = wire.full_name + config.wire_suffix[block.name][wire.name];
            } else {
                wire.full_name = wire.full_name + block_suffix;
            }
            // wire_name
            if (config.wire_name[block.name].count(wire.name)) {
                wire.code_name = config.wire_name[block.name][wire.name];
            } else {
                wire.code_name = wire.full_name;
            }

            wire.module_name = config.FindSubmoduleOfWire(block.name, wire.name);
            modules[wire.module_name].wires.emplace_back(wire);
        }
    }

    vga_size = config.template_width * config.template_height;
    vga_size_pow2 = 1;
    vga_size_log2 = 0;
    while (vga_size_pow2 < vga_size) {
        vga_size_pow2 <<= 1;
        ++vga_size_log2;
    }
}

void VgaDebugGenerator::ProcessModules(const std::string &name) {
    auto &module = modules[name];
    module.wires_all = module.wires;

    for (const auto &submodule_name : module.submodule_names) {
        ProcessModules(submodule_name);
        const auto &submodule = modules[submodule_name];
        for (const auto &wire : submodule.wires_all) {
            module.wires_all.emplace_back(wire);
        }
    }
}

void VgaDebugGenerator::Generate() {
    Generate_Mem();
    Generate_VgaDebugger();
    Generate_VgaDisplay();

    std::ofstream fout(config.output_dir + config.dbg_header);
    if (!fout) {
        throw "Failed to open debug header file '" + config.dbg_header + "'";
    }

    fout << "// generated code";
    Generate_VgaInstance(fout);
    Generate_Modules(fout);
}

void VgaDebugGenerator::Generate_Mem() {
    std::ofstream fout(config.output_dir + config.mem_file);
    if (!fout) {
        throw "Failed to open memory file '" + config.mem_file + "'";
    }

    int curr = 0;
    for (const auto &line : templte.lines) {
        for (int i = 0; i < config.template_width; i++) {
            if (i < line.size()) {
                fout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(line[i]) << std::endl;
            } else {
                fout << "00" << std::endl;
            }
            ++curr;
        }
    }
    while (curr < vga_size_pow2) {
        fout << "00" << std::endl;
        ++curr;
    }
}
void VgaDebugGenerator::Generate_VgaDebugger() {
    std::ofstream fout(config.output_dir + "VgaDebugger.v");
    if (!fout) {
        throw "Failed to open memory file 'VgaDebugger.v'";
    }

    fout << "// generated code\n" << std::endl;
    fout << "module Hex2Ascii(" << std::endl;
    fout << "    input wire [3:0] hex," << std::endl;
    fout << "    output reg [7:0] ascii" << std::endl;
    fout << ");\n" << std::endl;
    fout << "    always @* begin" << std::endl;
    fout << "        case (hex)" << std::endl;
    fout << "            4'h0: ascii = 48;" << std::endl;
    fout << "            4'h1: ascii = 49;" << std::endl;
    fout << "            4'h2: ascii = 50;" << std::endl;
    fout << "            4'h3: ascii = 51;" << std::endl;
    fout << "            4'h4: ascii = 52;" << std::endl;
    fout << "            4'h5: ascii = 53;" << std::endl;
    fout << "            4'h6: ascii = 54;" << std::endl;
    fout << "            4'h7: ascii = 55;" << std::endl;
    fout << "            4'h8: ascii = 56;" << std::endl;
    fout << "            4'h9: ascii = 57;" << std::endl;
    fout << "            4'ha: ascii = 97;" << std::endl;
    fout << "            4'hb: ascii = 98;" << std::endl;
    fout << "            4'hc: ascii = 99;" << std::endl;
    fout << "            4'hd: ascii = 100;" << std::endl;
    fout << "            4'he: ascii = 101;" << std::endl;
    fout << "            4'hf: ascii = 102;" << std::endl;
    fout << "        endcase" << std::endl;
    fout << "    end\n" << std::endl;
    fout << "endmodule\n" << std::endl;

    fout << "module VgaDebugger(" << std::endl;
    for (const auto &wire : modules[config.module_name].wires_all) {
        if (wire.len_bits == 1) {
            fout << "    input wire " << wire.full_name << "," << std::endl;
        } else {
            fout << "    input wire [" << wire.len_bits - 1 << ":0] " << wire.full_name << "," << std::endl;
        }
    }
    fout << "    input wire clk," << std::endl;
    fout << "    output reg display_wen," << std::endl;
    fout << "    output wire [" << vga_size_log2 - 1 <<  ":0] display_w_addr," << std::endl;
    fout << "    output wire [7:0] display_w_data" << std::endl;
    fout << ");\n" << std::endl;

    fout << "    reg [" << vga_size_log2 - 1 << ":0] display_addr = 0;" << std::endl;
    fout << "    assign display_w_addr = display_addr;" << std::endl;
    fout << "    always @(posedge clk) begin" << std::endl;
    fout << "        display_addr <= display_addr == " << vga_size - 1 << " ? 0 : display_addr + 1;" << std::endl;
    fout << "    end\n" << std::endl;

    fout << "    reg [3:0] dynamic_hex = 0;" << std::endl;
    fout << "    Hex2Ascii hex2ascii(dynamic_hex, display_w_data);" << std::endl;
    fout << "    always @* begin" << std::endl;
    fout << "        case (display_addr)" << std::endl;

    for (const auto &wire : modules[config.module_name].wires_all) {
        for (int i = 0; i < wire.len_hex; i++) {
            fout << "            " << wire.temp_start_pos + i << ": begin ";
            int lb = std::min(wire.len_bits, (wire.len_hex - i) * 4) - 1;
            int rb = (wire.len_hex - i - 1) * 4;
            if (lb == 0)  {
                fout << "dynamic_hex = " << wire.full_name << "; ";
            } else {
                fout << "dynamic_hex = " << wire.full_name << "[" << lb << ":" << rb << "]; ";
            }
            fout << "display_wen = 1; end" << std::endl;
        }
    }

    fout << "            default: begin dynamic_hex = 0; display_wen = 0; end" << std::endl;
    fout << "        endcase" << std::endl;
    fout << "    end\n" << std::endl;

    fout << "endmodule" << std::endl;
}
void VgaDebugGenerator::Generate_VgaDisplay() {

}
void VgaDebugGenerator::Generate_VgaInstance(std::ofstream &fout) {
    fout << "\n\n`define VGA_DBG_VgaDebugger_Arguments";
    for (const auto &wire : modules[config.module_name].wires_all) {
        fout << " \\\n    ." << wire.full_name << "(dbg_" << wire.full_name << "),";
    }
}

void VgaDebugGenerator::Generate_Modules(std::ofstream &fout) {
    for (const auto &[_, module] : modules) {
        Generate_Outputs(module, fout);
        Generate_Assignments(module, fout);
        Generate_Declaration(module, fout);
        Generate_Arguments(module, fout);
    }
}
void VgaDebugGenerator::Generate_Outputs(const Module &module, std::ofstream &fout) {
    fout << "\n\n`define VGA_DBG_" << module.name << "_Outputs";
    for (const auto &wire : module.wires_all) {
        fout << " \\\n    output wire ";
        if (wire.len_bits > 1) {
            fout << "[" << wire.len_bits - 1 << ":0] ";
        }
        fout << "dbg_" << wire.full_name << ",";
    }
}
void VgaDebugGenerator::Generate_Assignments(const Module &module, std::ofstream &fout) {
    fout << "\n\n`define VGA_DBG_" << module.name << "_Assignments";
    for (const auto &wire : module.wires) {
        fout << " \\\n    assign dbg_" << wire.full_name << " = " << wire.code_name << ";";
    }
}
void VgaDebugGenerator::Generate_Arguments(const Module &module, std::ofstream &fout) {
    fout << "\n\n`define VGA_DBG_" << module.name << "_Arguments";
    for (const auto &wire : module.wires_all) {
        fout << " \\\n    .dbg_" << wire.full_name << "(dbg_" << wire.full_name << "),";
    }
}
void VgaDebugGenerator::Generate_Declaration(const Module &module, std::ofstream &fout) {
    fout << "\n\n`define VGA_DBG_" << module.name << "_Declaration";
    for (const auto &wire : module.wires) {
        fout << " \\\n    wire ";
        if (wire.len_bits > 1) {
            fout << "[" << wire.len_bits - 1 << ":0] ";
        }
        fout << "dbg_" << wire.full_name << ";";
    }
}
