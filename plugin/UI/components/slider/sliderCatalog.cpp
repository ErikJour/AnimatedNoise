#include "sliderCatalog.h"
#include "ParamIds.h"
#include <cmath>


const std::vector<SliderDef>& sliderDefinitions()
{
    static const std::vector<SliderDef> defs = {
        { ParameterID::gain,   0.0f,  MAT_COMB_AMT_SLIDER},
    };
    return defs;
}
