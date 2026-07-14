#include "sliderCatalog.h"
#include "ParamIds.h"
#include <cmath>


const std::vector<SliderDef>& sliderDefinitions()
{
    static const std::vector<SliderDef> defs = {
        { ParameterID::gain,   0.0f,  MAT_GLOBAL_GAIN_SLIDER},
        // { ParameterID::noiseLevel,   0.0f,  MAT_GLOBAL_GAIN_SLIDER },
        // { ParameterID::noiseDensity, 1.45f, MAT_NOIS_DENS_SLIDER },
        // { ParameterID::lpgResonance, 2.975f, MAT_LPG_REZ_SLIDER }
    };
    return defs;
}
