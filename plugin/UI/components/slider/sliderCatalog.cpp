#include "sliderCatalog.h"
#include "ParamIds.h"
#include <cmath>


const std::vector<SliderDef>& sliderDefinitions()
{
    constexpr float topRow    = 0.15f;
    constexpr float bottomRow = -0.045f;
    static const std::vector<SliderDef> defs = {
        //==================================================
        //Noise Level: Radius = 2.0, angle = 0.0
        //==================================================
        { ParameterID::noiseLevel,
            0.0f,
            MAT_NOIS_LEVEL_SLIDER,
            { 2.0f, topRow, 0.0f },
            0.07f },
        //==================================================
        //Noise Level Mod: Radius = 2.0, angle = 0.0
        //==================================================
        { ParameterID::noiseLevelMod,
            0.0f,
            MAT_NOISE_LEVEL_MOD_SLIDER,
            { 2.0f, bottomRow, 0.0f },
            0.07f },
        //==================================================
        //Noise Density: Radius = 2.0, angle = 7.0
        //==================================================
        { ParameterID::noiseDensity,
            0.0f,
            MAT_NOIS_DENS_SLIDER,
                { 1.985f, topRow, 0.244f },
            0.07f },
        //==================================================
        //Noise Density Mod: Radius = 2.0, angle = 7.0
        //==================================================
        { ParameterID::noiseDensityMod,
            0.0f,
            MAT_NOISE_DENS_MOD_SLIDER,
                { 1.985f, bottomRow, 0.244f },
            0.07f },
        //==================================================
        //Env Attack: Radius = 2.0, angle = 67 (60 for other column)
        //==================================================
        { ParameterID::envAttack,
            0.0f,
            MAT_ATTACK_SLIDER,
                { .78f, topRow, 1.891f },
            0.07f },
        //==================================================
        //Env Decay: Radius = 2.0, angle = 60 , top row
        //==================================================
        { ParameterID::envDecay,
            0.0f,
            MAT_DECAY_SLIDER,
                { 1.f, topRow, 1.732f },
            0.07f },
        //==========================================================
        //Env Sustain: Radius = 2.0, angle = 67 (60 for other column)
        //==========================================================
        { ParameterID::envSustain,
            0.0f,
            MAT_SUSTAIN_SLIDER,
                { .78f, bottomRow, 1.891f },
            0.07f },
        //==================================================
        //Env Release: Radius = 2.0, angle = 60 , bottom row
        //==================================================
        { ParameterID::envRelease,
            0.0f,
            MAT_RELEASE_SLIDER,
                { 1.f, bottomRow, 1.732f },
            0.07f },
        //==========================================================
        //Filter Coeff A: Radius = 2.0, angle = 127 (60 for other column)
        //==========================================================
        { ParameterID::coeffA,
            0.0f,
            MAT_FILTER_ONE_SLIDER,
                { -1.203f, topRow, 1.597f },
            0.07f },
        //==================================================
        //Filter Coeff B: Radius = 2.0, angle = 120 , bottom row
        //==================================================
        { ParameterID::coeffB,
            0.0f,
            MAT_FILTER_TWO_SLIDER,
                { -1.f, topRow, 1.732f },
            0.07f },
    };
    return defs;
}

//Calculate x and z based on angle:
//x = radius * cos(angle)
//z = radius * sin(angle)
