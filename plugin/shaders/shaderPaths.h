//
// Created by Erik Jourgensen on 6/26/26.
//
#pragma once
#include <filesystem>
#include <string>

    inline std::vector<std::filesystem::path> getShaderPaths()
    {
        const std::string dir = DEBUG_SHADER_DIR;
        std::vector<std::filesystem::path> shaderPaths = {
            dir + "/shader_materials/common.wgsl",
            dir + "/shader_materials/lighting.wgsl",
            dir + "/shader_materials/mat_text.wgsl",
            dir + "/shader_materials/mat_slider.wgsl",
            dir + "/shader_materials/mat_gain_slider.wgsl",
            dir + "/shader_materials/mat_plane.wgsl",
            dir + "/shader_materials/mat_particle.wgsl",
            dir + "/shader_materials/mat_floor.wgsl",
            dir + "/shader_materials/mat_skylight.wgsl",
            dir + "/shader_materials/vs_main.wgsl",
            dir + "/shader_materials/fs_main.wgsl",
            dir + "/shader_materials/mat_lpg_rez_slider.wgsl",
            dir + "/shader_materials/mat_noise_density_slider.wgsl",
            dir + "/shader_materials/mat_logo.wgsl"
        };
        return shaderPaths;
    }




