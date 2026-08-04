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
            dir + "/shader_materials/global/common.wgsl",
            dir + "/shader_materials/lights/lighting.wgsl",
            dir + "/shader_materials/components/mat_text.wgsl",
            dir + "/shader_materials/sliders/mat_noise_density_mod_slider.wgsl",
            dir + "/shader_materials/sliders/mat_noise_level_slider.wgsl",
            dir + "/shader_materials/level/mat_plane.wgsl",
            dir + "/shader_materials/components/mat_particle.wgsl",
            dir + "/shader_materials/level/mat_level.wgsl",
            dir + "/shader_materials/level/mat_skylight.wgsl",
            dir + "/shader_materials/global/vs_main.wgsl",
            dir + "/shader_materials/global/fs_main.wgsl",
            dir + "/shader_materials/sliders/mat_noise_level_mod_slider.wgsl",
            dir + "/shader_materials/sliders/mat_noise_density_slider.wgsl",
            dir + "/shader_materials/components/mat_logo.wgsl",
            dir + "/shader_materials/components/mat_tooltip.wgsl",
            dir + "/shader_materials/lights/ambientLight.wgsl",
            dir + "/shader_materials/lights/mat_light_helper.wgsl",
            dir + "/shader_materials/sliders/mat_attack_slider.wgsl",
            dir + "/shader_materials/sliders/mat_decay_slider.wgsl",
            dir + "/shader_materials/sliders/mat_sustain_slider.wgsl",
            dir + "/shader_materials/sliders/mat_release_slider.wgsl"


        };
        return shaderPaths;
    }




