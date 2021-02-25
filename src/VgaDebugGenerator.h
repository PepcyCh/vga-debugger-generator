#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Config.h"
#include "Template.h"
#include "Wire.h"

class VgaDebugGenerator {
private:
    Config config;
    Template templte;
    std::unordered_map<std::string, Module> modules;
    int vga_size;
    int vga_size_pow2;
    int vga_size_log2;

public:
    void Run(const std::string &config_file);

private:
    void LoadConfig(const std::string &config_file);

    void LoadTemplate();

    void ProcessConfig();

    void ProcessModules(const std::string &name);

    void Generate();
    void Generate_Mem();
    void Generate_VgaDebugger();
    void Generate_VgaDisplay();
    void Generate_VgaInstance(std::ofstream &fout);

    void Generate_Modules(std::ofstream &fout);
    void Generate_Outputs(const Module &module, std::ofstream &fout);
    void Generate_Assignments(const Module &module, std::ofstream &fout);
    void Generate_Arguments(const Module &module, std::ofstream &fout);
    void Generate_Declaration(const Module &module, std::ofstream &fout);
};
