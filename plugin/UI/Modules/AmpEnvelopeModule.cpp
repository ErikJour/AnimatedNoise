//
// Created by Erik Jourgensen on 9/9/26.
//

#include "AmpEnvelopeModule.h"
#include "adsrGeometry.h"
#include "tubeGeometry.h"
#include <algorithm>

void AmpEnvelopeModule::build()
{
    std::vector<TubeVertex> vertices;
    std::vector<TubeIndex>  indices;

    AdsrGeometry::buildRampGrid(vertices, indices);
    AdsrGeometry::buildCaps(vertices, indices);

    mMesh.materialId = MAT_ADSR;
    mMesh.upload(mDevice, mQueue, vertices, indices);
}

void AmpEnvelopeModule::render(const MeshRenderer& renderer) const
{
    renderer.draw(mMesh);
}

//==========================================================================================
//The ramp's shape. Segment lengths are relative, so what reaches the shader is
//the four normalised to sum to 1 — the absolute times mean nothing to it.
//==========================================================================================
void AmpEnvelopeModule::writeUniforms(MyUniforms& uniforms) const
{
    //remapDecay / remapSustain in ampEnvelope.js. Decay and sustain are given
    //a floor so neither segment can shrink to nothing on the ramp, which is a
    //different thing from the minimum every segment gets below: this shapes
    //how the control reads, that one stops the curve degenerating.
    constexpr float kDecayFloor   = 18.0f, kDecaySpan   = 82.0f;
    constexpr float kSustainFloor = 12.0f, kSustainSpan = 88.0f;
    constexpr float kFullScale    = 100.0f;

    float a = mParameters.attack;
    float d = kDecayFloor   + (mParameters.decay   / kFullScale) * kDecaySpan;
    float s = kSustainFloor + (mParameters.sustain / kFullScale) * kSustainSpan;
    float r = mParameters.release;

    normaliseSegments(a, d, s, r);

    uniforms.adsrShape[0] = a;
    uniforms.adsrShape[1] = d;
    uniforms.adsrShape[2] = s;
    uniforms.adsrShape[3] = r;

    uniforms.adsrDims[0]  = AdsrGeometry::kWidth;
    uniforms.adsrDims[1]  = AdsrGeometry::kHeight;
    uniforms.adsrDims[2]  = AdsrGeometry::kTubeRadius;
    //sustainLevel is the sustain slider outright — how high the ramp holds,
    //rather than how long it holds for.
    uniforms.adsrDims[3]  = std::clamp(mParameters.sustain / kFullScale, 0.0f, 1.0f);
}

void AmpEnvelopeModule::normaliseSegments(float& a, float& d, float& s, float& r)
{
    //A segment below this vanishes into the tube radius and the ramp loses its
    //corner, so each gets a floor before the four are renormalised.
    constexpr float kMinSegment = 0.05f;

    float total = a + d + s + r;
    if (total <= 0.0f) total = 1.0f;

    a = std::max(a / total, kMinSegment);
    d = std::max(d / total, kMinSegment);
    s = std::max(s / total, kMinSegment);
    r = std::max(r / total, kMinSegment);

    const float floored = a + d + s + r;
    a /= floored; d /= floored; s /= floored; r /= floored;
}

void AmpEnvelopeModule::release()
{
    mMesh.release();
}
