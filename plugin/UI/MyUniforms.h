#pragma once
#include <cstddef>
#include <cstdint>

// Must match common.wgsl
static constexpr uint32_t MAT_CAVE                = 0;
static constexpr uint32_t MAT_GLOBAL_GAIN_SLIDER  = 1;
static constexpr uint32_t MAT_COMB_AMT_SLIDER     = 2;
static constexpr uint32_t MAT_PLANE               = 3;
static constexpr uint32_t MAT_PARTICLES           = 4;
static constexpr uint32_t MAT_FLOOR               = 5;
static constexpr uint32_t MAT_SKYLIGHT            = 6;
static constexpr uint32_t MAT_LPG_REZ_SLIDER      = 7;
static constexpr uint32_t MAT_NOIS_DENS_SLIDER    = 8;

struct MyUniforms {
    float    time;          // offset 0
    float    frequency;     // offset 4
    float    amplitude;     // offset 8
    float    sliderValue;   // offset 12
    float    lightPos[3];   // offset 20
    float    aspectRatio;   // offset 16
    float    pad[3];        // offset 32
    uint32_t materialId;    // offset 44
    float    modelMatrix[16];    // offset 48
    float    viewProjMatrix[16]; // offset 112
    float    projMatrix[16];     // offset 176  ← new
    float morph;           // offset 240
    float pad2[3];         // offset 244 → sizeof = 256
};

static_assert(sizeof(MyUniforms) % 16 == 0);
static_assert(offsetof(MyUniforms, modelMatrix) % 16 == 0);
static_assert(offsetof(MyUniforms, viewProjMatrix) % 16 == 0);

static constexpr float kIdentity[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,  // col 0
    0.0f, 1.0f, 0.0f, 0.0f,  // col 1
    0.0f, 0.0f, 1.0f, 0.0f,  // col 2
    0.0f, 0.0f, 0.0f, 1.0f   // col 3
};