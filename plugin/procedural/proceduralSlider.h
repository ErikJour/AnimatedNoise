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
    const float wallRadius     = 0.9f,
    const float wallAngle      = 0.0f,
    const int   curveVariant   = 0,
    const float roomX          = 0.0f,   // world-space room center offset
    const float roomZ          = 0.0f,
    const float yBottom        = -0.1875f,
    const float yTop           =  0.38125f,
    const int   heightSegments = 8,
    const int   radialSegments = 24)
{
    constexpr float kPI = 3.14159265f;

    const float cosA = std::cosf(wallAngle);
    const float sinA = std::sinf(wallAngle);

    const vec3 anchor  { roomX + wallRadius * cosA, 0.0f, roomZ + wallRadius * sinA };
    const vec3 radial  { cosA,  0.0f, sinA };
    const vec3 tangent { -sinA, 0.0f, cosA };

    struct Variant { float ampMod, xFreq, zFreq, phase; };

    const Variant variants[5] = {
        { 1.00f, 2.0f, 1.0f, 0.0f        },
        { 1.05f, 1.5f, 0.8f, kPI * 0.5f  },
        { 0.95f, 1.5f, 0.8f, -kPI * 0.5f },
        { 1.10f, 2.5f, 1.2f, kPI * 0.25f },
        { 0.90f, 1.0f, 1.5f, kPI * 0.75f },
    };
    const auto& vp = variants[curveVariant % 5];

    const float height            = yTop - yBottom;
    const float amplitude         = 0.00625f * vp.ampMod;  // +25% waviness
    constexpr float spineR        = 0.0065f;  // 50% of original (was 0.013)
    constexpr float indicatorR    = 0.013f;   // original width — indicator ring stays fat

    // Spine centerline at height-fraction t (0 = bottom, 1 = top).
    // Adds gentle side-to-side waviness that fades near both ends so caps sit clean.
    auto spineAt = [&](float t) -> vec3 {
        const float y        = yBottom + t * height;
        const float endCurve = 0.3f + std::sinf(t * kPI) * 0.7f;
        const float wRad     = std::sinf(t * kPI * vp.xFreq + vp.phase) * amplitude * endCurve;
        const float wTan     = std::cosf(t * kPI * vp.zFreq)            * amplitude * 0.5f * endCurve;
        return vec3{ anchor.x, y, anchor.z } + radial * wRad + tangent * wTan;
    };

    // Organic bulge only — no end taper (hemispherical caps handle the rounding).
    // sin(0) = sin(4π) = 0, so body radius exactly matches cap equator at both ends.
    auto scaledRadius = [&](float t, float baseR) -> float {
        const float bulge = 1.0f + std::sinf(t * kPI * 4.0f) * 0.1f;
        return baseR * bulge;
    };

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

    // Build a tube with hemispherical end caps.
    // Ring layout: [nCap bottom-cap rings] [nY+1 body rings] [nCap top-cap rings]
    // Cap phi = 0 at equator (full radius, meets body) → PI/2 at pole (radius 0).
    // Sphere normal = radialDir * cos(phi) + capDir * sin(phi).
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
            const float phi    = static_cast<float>(k) / nCap * kPI * 0.5f;
            const float sinPhi = std::sinf(phi);
            const float ringR  = baseR * std::cosf(phi);
            emitRing(base.pos - base.tangent * (baseR * sinPhi),
                     base.b1, base.b2,
                     ringR, 0.0f,
                     base.tangent * -1.0f, sinPhi);
        }

        // Main body: t = 0 → 1
        for (int j = 0; j <= heightSegments; ++j)
        {
            const float t = static_cast<float>(j) / static_cast<float>(heightSegments);
            const Frame f = getFrame(t);
            emitRing(f.pos, f.b1, f.b2,
                     scaledRadius(t, baseR), t,
                     vec3{0.0f, 0.0f, 0.0f}, 0.0f);
        }

        // Top cap: near-equator (k=1) → pole (k=nCap). capDir = +tangent.
        for (int k = 1; k <= nCap; ++k)
        {
            const float phi    = static_cast<float>(k) / nCap * kPI * 0.5f;
            const float sinPhi = std::sinf(phi);
            const float ringR  = baseR * std::cosf(phi);
            emitRing(top.pos + top.tangent * (baseR * sinPhi),
                     top.b1, top.b2,
                     ringR, 1.0f,
                     top.tangent, sinPhi);
        }

        // Quads connecting all adjacent rings — pure index arithmetic, unchanged.
        const int totalRings = nCap + (heightSegments + 1) + nCap;
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
