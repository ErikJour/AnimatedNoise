#ifndef ANIMATEDNOISE_SLIDERCATALOG_H
#define ANIMATEDNOISE_SLIDERCATALOG_H

#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdint>
#include <vector>
#include "../../../shaders/MyUniforms.h"


// ─────────────────────────────────────────────────────────────────────────────
// Slider catalog — the single source of truth for every slider.
//
// One row fully specifies a slider, and both consumers read these same rows:
//   • SliderManager — binds the APVTS param, runs hit-testing & drag gestures
//   • Scene         — builds the slider's geometry and draws it
//
// Geometry, hit-test, glow slot and material slot all derive from one row, so a
// slider is relocated or retuned by editing exactly one place (SliderCatalog.cpp).
// ─────────────────────────────────────────────────────────────────────────────
struct SliderDef
{
    juce::ParameterID paramID;       // APVTS parameter this slider drives
    float             angle;         // local angle within that room
    int               curveVariant;  // procedural waviness variant (0..4)
    std::uint32_t     materialId;    // MAT_* uniform / material slot
    int               glowIndex;     // position in the shader's sliderLevels[]
};

// The catalog, built lazily on first call. It MUST be a function-local static
// (not a namespace-scope variable): the rows copy juce::ParameterID, whose
// juce::String depends on JUCE's string pool, and the ParameterID constants live
// in ParamIds.h. Building at static-init time races that ordering and crashes
// before main(). Calling this at runtime sidesteps the order-of-init fiasco.
const std::vector<SliderDef>& sliderDefinitions();
void sliderGlowAnchor(float angle, float wallR, float yCenter,
                      float& outX, float& outY, float& outZ);

#endif // ANIMATEDNOISE_SLIDERCATALOG_H
