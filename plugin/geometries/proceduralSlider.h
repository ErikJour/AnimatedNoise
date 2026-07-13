#ifndef ANIMATEDNOISE_PROCEDURALSLIDER_H
#define ANIMATEDNOISE_PROCEDURALSLIDER_H
#include <vector>
#include <cmath>
#include "geometryMath.h"

struct SliderVertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v, pad;
};
using SliderIndex = uint16_t;

inline void buildSliderGeometry(
    std::vector<SliderVertex>& verts,
    std::vector<SliderIndex>&  indices,
    const float wallRadius     = 0.9f,      //where the slider will be in terms of a circle
    const float wallAngle      = 0.0f,      //not sure
    const int   curveVariant   = 0,         //variation in curvature, static
    const float roomX          = 0.0f,      //static
    const float roomZ          = 0.0f,      //static
    const float yBottom        = 0.f,  //static, changing this messes up the mouse tracking
    const float yTop           = 0.03f, //static, changing this messes up the mouse tracking
    const int   radialSegments = 24)        //static
{
    constexpr float kPI = 3.14159265f;      //pi

    const float cosA = std::cosf(wallAngle);//gives us our location I think
    const float sinA = std::sinf(wallAngle);//gives us our location I think

    const vec3 anchor  { roomX + wallRadius * cosA, 0.0f, roomZ + wallRadius * sinA };
    const vec3 radial  { cosA,  0.0f, sinA };
    const vec3 tangent { -sinA, 0.0f, cosA };

    struct Variant { float ampMod, xFreq, zFreq, phase; };

    //These are the curve variants===================================
    const Variant variants[5] = {
        { 1.00f, 2.0f, 1.0f, 0.0f        },
        { 1.05f, 1.5f, 0.8f, kPI * 0.5f  },
        { 0.95f, 1.5f, 0.8f, -kPI * 0.5f },
        { 1.10f, 2.5f, 1.2f, kPI * 0.25f },
        { 0.90f, 1.0f, 1.5f, kPI * 0.75f },
    };
    const auto& [ampMod, xFreq, zFreq, phase] = variants[curveVariant % 5];
    //================================================================

    const float height            = yTop - yBottom;
    const float amplitude         = 0.000f * ampMod;  // +25% waviness
    constexpr float spineR        = 0.03f;  // 50% of original (was 0.013)
    constexpr float indicatorR    = 0.01f;   // original width — indicator ring stays fat

    // Spine centerline at height-fraction t (0 = bottom, 1 = top).
    // Adds gentle side-to-side waviness that fades near both ends so caps sit clean.
    auto spineAt = [&](float t) -> vec3 {
        const float y        = yBottom + t * height;
        const float endCurve = 0.01f + std::sinf(t * kPI) * 0.01f;
        const float wRad     = std::sinf(t * kPI * xFreq + phase) * amplitude * endCurve;
        const float wTan     = std::cosf(t * kPI * zFreq)            * amplitude * 0.5f * endCurve;
        return vec3{ anchor.x, y, anchor.z } + radial * wRad + tangent * wTan;
    };

    // Organic bulge only — no end taper (hemispherical caps handle the rounding).
    // sin(0) = sin(4π) = 0, so body radius exactly matches cap equator at both ends.

    // Frenet frame at spine parameter t — shared by body rings and cap placement.
    auto getFrame = [&](float t) -> Frame {
        const vec3  here = spineAt(t);
        const float eps  = 0.005f;
        const vec3  tan  = (spineAt(std::min(t + eps, 1.0f))
                          - spineAt(std::max(t - eps, 0.0f))).normalized();
        const vec3  b1   = (radial - tan * radial.dot(tan)).normalized();
        const vec3  b2   = tan.cross(b1);
        return { here, tan, b1, b2 };
    };

    auto buildTube = [&](float baseR, float padVal)
    {
        const int  nCap  = 6;
        const auto vBase = static_cast<SliderIndex>(verts.size());
        const int  row   = radialSegments + 1;

        const Frame base = getFrame(0.0f);   // bottom endpoint
        const Frame top  = getFrame(1.0f);   // top endpoint

        // Emit one ring of nRad+1 vertices around `center`, swept in the a1/a2 plane.
        // Body rings pass capDir = 0 and capSinPhi = 0 (normals point straight out);
        // cap rings blend the outward normal toward capDir to round the hemisphere.
        auto emitRing = [&](vec3 center, vec3 a1, vec3 a2,
                            float ringR, float vCoord,
                            vec3 capDir, float capSinPhi)
        {
            const float capCosPhi = std::sqrtf(std::max(0.0f, 1.0f - capSinPhi * capSinPhi));
            for (int i = 0; i <= radialSegments; ++i)
            {
                const float uCoord = static_cast<float>(i) / static_cast<float>(radialSegments);
                const float phi    = uCoord * 2.0f * kPI;
                const float cp = std::cosf(phi), sp = std::sinf(phi);

                const vec3 r = a1 * cp + a2 * sp;
                const vec3 n = (r * capCosPhi + capDir * capSinPhi).normalized();
                const vec3 p = center + r * ringR;

                SliderVertex sv{};
                sv.x = p.x;  sv.y = p.y;  sv.z = p.z;
                sv.nx = n.x; sv.ny = n.y; sv.nz = n.z;
                sv.u = uCoord; sv.v = vCoord; sv.pad = padVal;
                verts.push_back(sv);
            }
        };

        // Bottom cap: pole (k=nCap) → near-equator (k=1). capDir = -tangent.
        for (int k = nCap; k >= 1; --k)
        {
            const float phi    = static_cast<float>(k) / nCap * kPI * 0.1f;
            const float sinPhi = std::sinf(phi);
            const float ringR  = baseR * std::cosf(phi);
            emitRing(base.pos - base.tangent * (baseR * sinPhi),
                     base.b1, base.b2,
                     ringR, 0.0f,
                     base.tangent * -1.0f, sinPhi);
        }



        // Top cap: near-equator (k=1) → pole (k=nCap). capDir = +tangent.
        for (int k = 1; k <= nCap; ++k)
        {
            const float phi    = static_cast<float>(k) / nCap * kPI * 0.1f;
            const float sinPhi = std::sinf(phi);
            const float ringR  = baseR * std::cosf(phi);
            emitRing(top.pos + top.tangent * (baseR * sinPhi),
                     top.b1, top.b2,
                     ringR, 1.0f,
                     top.tangent, sinPhi);
        }

        // Quads connecting all adjacent rings — pure index arithmetic, unchanged.
        const int totalRings = nCap + (1) + nCap;
        for (int r = 0; r < totalRings - 1; ++r)
            for (int i = 0; i < radialSegments; ++i)
            {
                const auto bl = static_cast<SliderIndex>(vBase + r       * row + i);
                const auto br = static_cast<SliderIndex>(vBase + r       * row + i + 1);
                const auto tl = static_cast<SliderIndex>(vBase + (r + 1) * row + i);
                const auto tr = static_cast<SliderIndex>(vBase + (r + 1) * row + i + 1);
                indices.insert(indices.end(), {bl, br, tl, br, tr, tl});
            }
    };

    buildTube(spineR,     0.0f);   // thin spine
    buildTube(indicatorR, 1.0f);   // fat indicator ring
}

#endif
