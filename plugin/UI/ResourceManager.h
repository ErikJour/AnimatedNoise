//
// Created by Erik Jourgensen on 5/6/26.
//

#ifndef ANIMATEDNOISE_RESOURCEMANAGER_H
#define ANIMATEDNOISE_RESOURCEMANAGER_H
#pragma once
#include <vector>
#include <filesystem>
#include <webgpu/webgpu.h>




class ResourceManager {
public:
    static bool loadGeometry(const std::filesystem::path& path,
                                   std::vector<float>& pointData,
                                   std::vector<uint16_t>& indexData
                                    );
    /**
 * Create a shader module for a given WebGPU `device` from a WGSL shader source
 * loaded from file `path`.
 */
    static WGPUShaderModule loadShaderModule(
        const std::filesystem::path& path,
        WGPUDevice device
    );

private:
    std::string                    mShaderSource;

};


#endif //ANIMATEDNOISE_RESOURCEMANAGER_H