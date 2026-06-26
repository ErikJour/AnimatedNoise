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
            dir + "/common.wgsl",
            dir + "/lighting.wgsl",
            dir + "/mat_cave.wgsl",
            dir + "/mat_slider.wgsl",
            dir + "/mat_comb_slider.wgsl",
            dir + "/mat_plane.wgsl",
            dir + "/mat_particle.wgsl",
            dir + "/mat_floor.wgsl",
            dir + "/mat_skylight.wgsl",
            dir + "/vs_main.wgsl",
            dir + "/fs_main.wgsl",
            dir + "/mat_lpg_rez_slider.wgsl",
            dir + "/mat_noise_density_slider.wgsl"
        };
        return shaderPaths;
    }




