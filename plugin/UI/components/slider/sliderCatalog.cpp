#include "sliderCatalog.h"
#include "ParamIds.h"
#include <cmath>


const std::vector<SliderDef>& sliderDefinitions()
{

    static const std::vector<SliderDef> defs = {
        { ParameterID::noiseLevel,
            0.0f,
            MAT_MASTER_GAIN_SLIDER,
            { 1.3f, 0.15f, -0.6f },
            0.07f },

        { ParameterID::noiseDensity,
            0.0f,
            MAT_NOIS_DENS_SLIDER,
            { 1.5f, 0.15f, -0.5f },
            0.07f },
    };





    return defs;
}
