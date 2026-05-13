#pragma once
#include <cstddef>
#include <cstdint>

// Must match common.wgsl
static constexpr uint32_t MAT_CAVE             = 0;
static constexpr uint32_t MAT_SLIDER           = 1;
static constexpr uint32_t MAT_PLANE            = 2;
static constexpr uint32_t MAT_PARTICLES        = 3;

struct MyUniforms {
    float    time;
    float    frequency;
    float    amplitude;
    float    sliderValue;
    float    lightPos[3];
    float    _pad1;
    float    sliderPos[3];
    uint32_t materialId;
    float    modelMatrix[16];

};

static_assert(sizeof(MyUniforms) % 16 == 0);
static_assert(offsetof(MyUniforms, modelMatrix) % 16 == 0);

static constexpr float kIdentity[16] = {
    1.0f,    0.0f,    0.0f,  0.0f,  // col 0
    0.0f,    0.7071f, -0.7071f, 0.0f,  // col 1
    0.0f,    0.7071f,  0.7071f, 0.0f,  // col 2
    0.0f,    0.0f,    0.0f,  1.0f   // col 3
};