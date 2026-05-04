//
// Created by Erik Jourgensen on 5/4/26.
//

#ifndef ANIMATEDNOISE_SHADERLOADER_H
#define ANIMATEDNOISE_SHADERLOADER_H

// shaderLoader.h
#include <fstream>
#include <sstream>
#include <string>

inline std::string loadShader(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Could not open shader: " + path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

#endif //ANIMATEDNOISE_SHADERLOADER_H