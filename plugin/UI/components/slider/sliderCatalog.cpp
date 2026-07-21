#include "sliderCatalog.h"
#include "ParamIds.h"
#include <cmath>


const std::vector<SliderDef>& sliderDefinitions()
{
    static const std::vector<SliderDef> defs = {
        //==================================================
        //Noise Level: Radius = 2.0, angle = 0.0
        //==================================================
        { ParameterID::noiseLevel,
            0.0f,
            MAT_NOIS_LEVEL_SLIDER,
            { 2.0f, 0.15f, 0.0f },
            0.07f },
        //==================================================
        //Noise Level Mod: Radius = 2.0, angle = 0.0, height:
        //==================================================
        { ParameterID::noiseLevelMod,
            0.0f,
            MAT_NOISE_LEVEL_MOD_SLIDER,
            { 2.0f, -0.045f, 0.0f },
            0.07f },
        //==================================================
        //Noise Density: Radius = 2.0, angle = 7.0
        //==================================================
        { ParameterID::noiseDensity,
            0.0f,
            MAT_NOIS_DENS_SLIDER,
                { 1.985f, 0.15f, 0.244f },
            0.07f },
        //==================================================
        //Noise Density Mod: Radius = 2.0, angle = 7.0, Height:
        //==================================================
        { ParameterID::noiseDensityMod,
            0.0f,
            MAT_NOISE_DENS_MOD_SLIDER,
                { 1.985f, -0.045f, 0.244f },
            0.07f },
    };
    return defs;
}

//Calculate x and z based on angle:
//x = radius * cos(angle)
//z = radiu s* sin(angle)
