//
// TubeGeometry — port of THREE.TubeGeometry (r150), plus the small merge and
// translate helpers that stand in for BufferGeometryUtils.mergeBufferGeometries
// and BufferGeometry.translate.
//
// UVs go in the colour slot (u, v, 0), matching cylinderGeometry.h and the
// engine's pos(3)/normal(3)/colour(3) vertex layout.
//
#pragma once
#include <vector>
#include <cmath>

#include "circularFloor.h"
#include "geometryMath.h"
#include "curves.h"
#include "sharedHelper.h"

struct TubeVertex
{
    float x, y, z;
    float nX, nY, nZ;
    float u, v, w;
};

using TubeIndex = uint16_t;

class TubeGeometry
{
public:
    static void buildTube(std::vector<TubeVertex>& vertices,
                          std::vector<TubeIndex>&  indices,
                          const Curve& path,
                          const int   tubularSegments = 64,
                          const float radius          = 1.0f,
                          const int   radialSegments  = 8,
                          const bool  closed          = false)
    {
        const auto frames = path.computeFrenetFrames(tubularSegments, closed);

        const auto baseVertex = static_cast<uint32_t>(vertices.size());

        //=========================================================
        //Vertices + normals (THREE generateSegment)
        //=========================================================
        const int lastRow = closed ? tubularSegments : tubularSegments;

        for (int i = 0; i <= lastRow; ++i)
        {
            const int frameIndex = (closed && i == tubularSegments) ? 0 : i;

            const vec3 P = path.getPointAt(static_cast<float>(i)
                                         / static_cast<float>(tubularSegments));

            const vec3 N = frames.normals[static_cast<size_t>(frameIndex)];
            const vec3 B = frames.binormals[static_cast<size_t>(frameIndex)];

            for (int j = 0; j <= radialSegments; ++j)
            {
                const float v = static_cast<float>(j)
                              / static_cast<float>(radialSegments) * 2.0f * PI;

                const float sinV =  std::sin(v);
                const float cosV = -std::cos(v);

                vec3 normal{ cosV * N.x + sinV * B.x,
                             cosV * N.y + sinV * B.y,
                             cosV * N.z + sinV * B.z };
                normal = normal.normalized();

                vertices.push_back({
                    P.x + radius * normal.x,
                    P.y + radius * normal.y,
                    P.z + radius * normal.z,
                    normal.x, normal.y, normal.z,
                    static_cast<float>(i) / static_cast<float>(tubularSegments),
                    static_cast<float>(j) / static_cast<float>(radialSegments),
                    0.0f
                });
            }
        }

        //=========================================================
        //Indices (THREE generateIndices)
        //=========================================================
        for (int j = 1; j <= tubularSegments; ++j)
        {
            for (int i = 1; i <= radialSegments; ++i)
            {
                const uint32_t a = baseVertex + static_cast<uint32_t>((radialSegments + 1) * (j - 1) + (i - 1));
                const uint32_t b = baseVertex + static_cast<uint32_t>((radialSegments + 1) *  j      + (i - 1));
                const uint32_t c = baseVertex + static_cast<uint32_t>((radialSegments + 1) *  j      +  i);
                const uint32_t d = baseVertex + static_cast<uint32_t>((radialSegments + 1) * (j - 1) +  i);

                indices.push_back(static_cast<TubeIndex>(a));
                indices.push_back(static_cast<TubeIndex>(b));
                indices.push_back(static_cast<TubeIndex>(d));

                indices.push_back(static_cast<TubeIndex>(b));
                indices.push_back(static_cast<TubeIndex>(c));
                indices.push_back(static_cast<TubeIndex>(d));
            }
        }
    }

    //=================================================================
    //Stand-in for THREE.SphereGeometry(radius, w, h).translate(x, y, z),
    //used as the end caps on every wave shape.
    //=================================================================
    static void appendSphereCap(std::vector<TubeVertex>& vertices,
                                std::vector<TubeIndex>&  indices,
                                const float radius,
                                const int   widthSegments,
                                const int   heightSegments,
                                const vec3  translate)
    {
        const auto baseVertex = static_cast<uint32_t>(vertices.size());

        for (int iy = 0; iy <= heightSegments; ++iy)
        {
            const float v = static_cast<float>(iy) / static_cast<float>(heightSegments);

            for (int ix = 0; ix <= widthSegments; ++ix)
            {
                const float u = static_cast<float>(ix) / static_cast<float>(widthSegments);

                const float px = -radius * std::cos(u * 2.0f * PI) * std::sin(v * PI);
                const float py =  radius * std::cos(v * PI);
                const float pz =  radius * std::sin(u * 2.0f * PI) * std::sin(v * PI);

                const vec3 n = vec3{ px, py, pz }.normalized();

                vertices.push_back({
                    px + translate.x, py + translate.y, pz + translate.z,
                    n.x, n.y, n.z,
                    u, 1.0f - v, 0.0f
                });
            }
        }

        const int rowWidth = widthSegments + 1;

        for (int iy = 0; iy < heightSegments; ++iy)
        {
            for (int ix = 0; ix < widthSegments; ++ix)
            {
                const uint32_t a = baseVertex + static_cast<uint32_t>( iy      * rowWidth + ix + 1);
                const uint32_t b = baseVertex + static_cast<uint32_t>( iy      * rowWidth + ix);
                const uint32_t c = baseVertex + static_cast<uint32_t>((iy + 1) * rowWidth + ix);
                const uint32_t d = baseVertex + static_cast<uint32_t>((iy + 1) * rowWidth + ix + 1);

                if (iy != 0)
                {
                    indices.push_back(static_cast<TubeIndex>(a));
                    indices.push_back(static_cast<TubeIndex>(b));
                    indices.push_back(static_cast<TubeIndex>(d));
                }
                if (iy != heightSegments - 1)
                {
                    indices.push_back(static_cast<TubeIndex>(b));
                    indices.push_back(static_cast<TubeIndex>(c));
                    indices.push_back(static_cast<TubeIndex>(d));
                }
            }
        }
    }

    //=================================================================
    //Translate every vertex — used to place a whole shape in the level.
    //=================================================================
    static void translate(std::vector<TubeVertex>& vertices, const vec3 offset)
    {
        for (auto& vert : vertices)
        {
            vert.x += offset.x;
            vert.y += offset.y;
            vert.z += offset.z;
        }
    }
};
