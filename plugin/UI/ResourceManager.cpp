//
// Created by Erik Jourgensen on 5/6/26.
//

#include "ResourceManager.h"

#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

bool ResourceManager::loadGeometry(const std::filesystem::path& path,
                                          std::vector<float>& pointData,
                                          std::vector<uint16_t>& indexData,
                                          int dimensions)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    pointData.clear();
    indexData.clear();

    enum class Section {
        None,
        Points,
        Indices,
    };
    Section currentSection = Section::None;

    float value;
    uint16_t index;
    std::string line;
    while (!file.eof()) {
        getline(file, line);

        // overcome the `CRLF` problem
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "[points]") {
            currentSection = Section::Points;
        }
        else if (line == "[indices]") {
            currentSection = Section::Indices;
        }
        else if (line[0] == '#' || line.empty()) {
            // Do nothing, this is a comment
        }
        else if (currentSection == Section::Points) {
            std::istringstream iss(line);
            // Get x, y, r, g, b
            for (int i = 0; i < dimensions + 6; ++i) {
                iss >> value;
                pointData.push_back(value);
            }
        }
        else if (currentSection == Section::Indices) {
            std::istringstream iss(line);
            // Get corners #0 #1 and #2
            for (int i = 0; i < 3; ++i) {
                iss >> index;
                indexData.push_back(index);
            }
        }
    }
    return true;
}

WGPUShaderModule ResourceManager::loadShaderModule(const std::filesystem::path& path, WGPUDevice device) {
    std::ifstream file(path);
    if (!file.is_open()) return nullptr;

    file.seekg(0, std::ios::end);
    const auto fileSize     = file.tellg();
    const auto size         = static_cast<size_t>(fileSize);
    std::string shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), static_cast<std::streamsize>(size));

    WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
    shaderCodeDesc.chain.next  = nullptr;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    shaderCodeDesc.code        = { shaderSource.c_str(), shaderSource.size() };

    WGPUShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    return wgpuDeviceCreateShaderModule(device, &shaderDesc);
}

WGPUShaderModule ResourceManager::loadShaderModules(const std::vector<std::filesystem::path>& paths,
                                                     WGPUDevice device)
{
    //API expects a continuous string of wgsl
    std::string combined;
    for (const auto& path : paths) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Could not open shader: " << path << std::endl;
            return nullptr;
        }
        file.seekg(0, std::ios::end);
        const auto size = static_cast<size_t>(file.tellg());
        std::string src(size, ' ');
        file.seekg(0);
        file.read(src.data(), static_cast<std::streamsize>(size));
        combined += src;
        combined += '\n';
    }
    //Configure shader
    WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
    shaderCodeDesc.chain.next  = nullptr;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    shaderCodeDesc.code        = { combined.c_str(), combined.size() };
    //Compile the shader into an object for the GPU
    WGPUShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    return wgpuDeviceCreateShaderModule(device, &shaderDesc);
}