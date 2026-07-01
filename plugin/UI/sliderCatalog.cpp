#include "sliderCatalog.h"
#include "ParamIds.h"
#include <cmath>


const std::vector<SliderDef>& sliderDefinitions()
{
    static const std::vector<SliderDef> defs = {
        { ParameterID::noiseLevel,   0.0f,  0, MAT_GLOBAL_GAIN_SLIDER, 0 },
        { ParameterID::noiseDensity, 0.725f, 1, MAT_NOIS_DENS_SLIDER,   1 },
        { ParameterID::lpgResonance, 1.45f, 2, MAT_LPG_REZ_SLIDER,   2 },
        { ParameterID::combLevel,    2.975f,  3, MAT_COMB_AMT_SLIDER,  3 },
    };
    return defs;
}

void sliderGlowAnchor(const float angle, const float wallR, const float yCenter,
                      float& outX, float& outY, float& outZ)
{
    outX = wallR * std::cos(angle);
    outY = yCenter;
    outZ = wallR * std::sin(angle);
}
