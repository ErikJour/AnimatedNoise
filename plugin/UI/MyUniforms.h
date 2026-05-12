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
};

static_assert(sizeof(MyUniforms) % 16 == 0);