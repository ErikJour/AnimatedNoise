#pragma once
#include <cstddef>
#include <cstdint>

// Must match common.wgsl
static constexpr uint32_t MAT_TEXT                = 0;
static constexpr uint32_t MAT_MASTER_GAIN_SLIDER  = 1;
static constexpr uint32_t MAT_COMB_AMT_SLIDER     = 2;
static constexpr uint32_t MAT_PLANE               = 3;
static constexpr uint32_t MAT_PARTICLES           = 4;
static constexpr uint32_t MAT_LEVEL               = 5;
static constexpr uint32_t MAT_SKYLIGHT            = 6;
static constexpr uint32_t MAT_LPG_REZ_SLIDER      = 7;
static constexpr uint32_t MAT_NOIS_DENS_SLIDER    = 8;
static constexpr uint32_t MAT_LOGO                = 9;
static constexpr uint32_t MAT_TOOLTIP             = 10;

struct MyUniforms {
    float    time;
    float    frequency;
    float    amplitude;
    float    sliderValue;
    float    lightPos[3];
    float    aspectRatio;
    float    sliderLevels[4];
    float    sliderGlowPos[16];
    float    modelMatrix[16];
    float    viewProjMatrix[16];
    float    projMatrix[16];
    float    morph;
    float    pressed;
    uint32_t materialId;
    float    resonate;
    float    sliderPosition[3];
    float    pad;
};

static_assert(sizeof(MyUniforms) % 16              == 0);
static_assert(offsetof(MyUniforms, sliderLevels)   == 32);
static_assert(offsetof(MyUniforms, sliderGlowPos)  == 48);
static_assert(offsetof(MyUniforms, modelMatrix)    == 112);
static_assert(offsetof(MyUniforms, materialId)     == 312);
static_assert(offsetof(MyUniforms, modelMatrix)    % 16 == 0);
static_assert(offsetof(MyUniforms, viewProjMatrix) % 16 == 0);

static constexpr float kIdentity[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};