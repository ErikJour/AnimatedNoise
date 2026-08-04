#pragma once
#include <cstddef>
#include <cstdint>

// Remember that these have to match the Common materials order
//===========================================
//Level
//===========================================
static constexpr uint32_t MAT_LEVEL                  = 0;
static constexpr uint32_t MAT_SKYLIGHT               = 1;
//===========================================
//Components
//===========================================
static constexpr uint32_t MAT_LOGO                   = 2;
static constexpr uint32_t MAT_TEXT                   = 3;
static constexpr uint32_t MAT_TOOLTIP                = 4;
//===========================================
//Visualizations
//===========================================
static constexpr uint32_t MAT_PARTICLES              = 5;
//===========================================
//Sliders
//===========================================
static constexpr uint32_t MAT_NOIS_LEVEL_SLIDER      = 6;
static constexpr uint32_t MAT_NOISE_LEVEL_MOD_SLIDER = 7;
static constexpr uint32_t MAT_NOIS_DENS_SLIDER       = 8;
static constexpr uint32_t MAT_NOISE_DENS_MOD_SLIDER  = 9;
static constexpr uint32_t MAT_ATTACK_SLIDER          = 10;
static constexpr uint32_t MAT_DECAY_SLIDER           = 11;
static constexpr uint32_t MAT_SUSTAIN_SLIDER         = 12;
static constexpr uint32_t MAT_RELEASE_SLIDER         = 13;
//============================
//Utilities
//============================
static constexpr uint32_t MAT_LIGHT_HELPER           = 14;
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
    float    cameraPosition[3];
    float    pad[2];
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