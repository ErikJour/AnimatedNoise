//
// ADSR ramp — port of TDS-01's WebUI/AmpEnvelope/ampGeometries.js.
//
// Note this module inverts the usual split: createTubeGeometry() emits a grid
// of ZERO positions with dummy normals, and adsrVertexShader.glsl builds the
// entire tube from the UV. So the geometry here is just the UV grid and its
// index pattern — the shape lives in mat_adsr.wgsl.
//
// UVs ride in the colour slot, as everywhere else in this port.
//
#pragma once
#include <vector>
#include <cmath>
#include "tubeGeometry.h"

class AdsrGeometry
{
public:
    // create3DADSRRamp defaults
    static constexpr int   kLengthSegments = 64;
    static constexpr int   kRadialSegments = 16;
    static constexpr float kWidth          = 3.5f;   // ampEnvelope.js
    static constexpr float kHeight         = 1.5f;
    static constexpr float kTubeRadius     = 0.19f;
    static constexpr float kTension        = 0.4f;

    //=================================================================
    //createTubeGeometry — positions are all zero by design.
    //=================================================================
    static void buildRampGrid(std::vector<TubeVertex>& vertices,
                              std::vector<TubeIndex>&  indices,
                              const int lengthSegments = kLengthSegments,
                              const int radialSegments = kRadialSegments)
    {
        const auto base = static_cast<uint32_t>(vertices.size());

        for (int i = 0; i <= lengthSegments; ++i)
        {
            const float u = static_cast<float>(i) / static_cast<float>(lengthSegments);

            for (int j = 0; j <= radialSegments; ++j)
            {
                const float v = static_cast<float>(j) / static_cast<float>(radialSegments);

                vertices.push_back({
                    0.0f, 0.0f, 0.0f,     // overridden in the vertex shader
                    0.0f, 1.0f, 0.0f,     // dummy
                    u, v, 0.0f
                });
            }
        }

        for (int i = 0; i < lengthSegments; ++i)
        {
            for (int j = 0; j < radialSegments; ++j)
            {
                const uint32_t a = base + static_cast<uint32_t>(i * (radialSegments + 1) + j);
                const uint32_t b = a + 1u;
                const uint32_t c = a + static_cast<uint32_t>(radialSegments + 1);
                const uint32_t d = c + 1u;

                indices.push_back(static_cast<TubeIndex>(a));
                indices.push_back(static_cast<TubeIndex>(b));
                indices.push_back(static_cast<TubeIndex>(c));

                indices.push_back(static_cast<TubeIndex>(b));
                indices.push_back(static_cast<TubeIndex>(d));
                indices.push_back(static_cast<TubeIndex>(c));
            }
        }
    }

    //=================================================================
    //End caps: createCapGeometry(radialSegments) is SphereGeometry(1, s, s),
    //scaled by tubeRadius and placed at -+width/2 (updateCaps). Width and
    //radius never change after construction, so these are baked.
    //
    //Flagged in the spare colour channel so the material can light them
    //without running the ramp's vertex displacement.
    //=================================================================
    static void buildCaps(std::vector<TubeVertex>& vertices,
                          std::vector<TubeIndex>&  indices,
                          const float width      = kWidth,
                          const float tubeRadius = kTubeRadius,
                          const int   segments   = kRadialSegments)
    {
        for (const float sign : { -1.0f, 1.0f })
        {
            std::vector<TubeVertex> cap;
            std::vector<TubeIndex>  capIdx;

            TubeGeometry::appendSphereCap(cap, capIdx, tubeRadius, segments, segments,
                                          { sign * width * 0.5f, 0.0f, 0.0f });

            for (auto& vert : cap) vert.w = 1.0f;   // cap flag

            const auto base = static_cast<uint32_t>(vertices.size());
            vertices.insert(vertices.end(), cap.begin(), cap.end());
            for (const auto i : capIdx)
                indices.push_back(static_cast<TubeIndex>(base + i));
        }
    }
};
