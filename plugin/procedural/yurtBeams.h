//
// Created by Erik Jourgensen on 5/22/26.
//

#ifndef YURTBEAMS_H
#define YURTBEAMS_H

#include <vector>
#include <cstdint>
#include <cmath>

struct BeamVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};

using BeamIndex = uint16_t;

class YurtBeams {
public:
    static void buildBeams(
        std::vector<BeamVertex>& verts,
        std::vector<BeamIndex>&  indices,
        float floorRadius    = 1.0f,
        float floorY         = -0.15f,
        float skylightRadius = 0.2f,
        float skylightY      = 0.5f,
        int   segments       = 32,
        float beamHalfWidth  = 0.025f,
        float beamHalfDepth  = 0.015f)
    {
        verts.clear();
        indices.clear();
        if (segments < 3) segments = 3;

        // 4 faces × 4 verts, 4 faces × 6 indices per beam
        verts.reserve(static_cast<size_t>(segments) * 16);
        indices.reserve(static_cast<size_t>(segments) * 24);

        const float PI = 3.14159265358979323846f;
        const float cr = 0.85f, cg = 0.78f, cb = 0.65f;

        for (int s = 0; s < segments; ++s)
        {
            const float theta = (float(s) / float(segments)) * 2.0f * PI;
            const float cosT  = std::cos(theta);
            const float sinT  = std::sin(theta);

            // Outward radial unit vector (XZ)
            const float rX = cosT, rZ = sinT;
            // Tangential unit vector (XZ, CCW)
            const float tX = -sinT, tZ = cosT;

            // Beam axis: bottom center → top center
            const float bX = cosT * floorRadius,    bY = floorY,    bZ = sinT * floorRadius;
            const float uX = cosT * skylightRadius, uY = skylightY, uZ = sinT * skylightRadius;

            const float w = beamHalfWidth;
            const float d = beamHalfDepth;

            // 8 corners: bottom/top × inner(−radial)/outer(+radial) × left(−tan)/right(+tan)
            float b_il_x = bX - w*tX - d*rX, b_il_z = bZ - w*tZ - d*rZ; // bottom inner-left
            float b_ir_x = bX + w*tX - d*rX, b_ir_z = bZ + w*tZ - d*rZ; // bottom inner-right
            float b_ol_x = bX - w*tX + d*rX, b_ol_z = bZ - w*tZ + d*rZ; // bottom outer-left
            float b_or_x = bX + w*tX + d*rX, b_or_z = bZ + w*tZ + d*rZ; // bottom outer-right

            float t_il_x = uX - w*tX - d*rX, t_il_z = uZ - w*tZ - d*rZ; // top inner-left
            float t_ir_x = uX + w*tX - d*rX, t_ir_z = uZ + w*tZ - d*rZ; // top inner-right
            float t_ol_x = uX - w*tX + d*rX, t_ol_z = uZ - w*tZ + d*rZ; // top outer-left
            float t_or_x = uX + w*tX + d*rX, t_or_z = uZ + w*tZ + d*rZ; // top outer-right

            // Emit one quad face — CCW from the direction the normal points
            auto emitFace = [&](
                float x0, float y0, float z0,
                float x1, float y1, float z1,
                float x2, float y2, float z2,
                float x3, float y3, float z3,
                float nx, float ny, float nz)
            {
                const auto base = static_cast<BeamIndex>(verts.size());
                verts.push_back({x0,y0,z0, nx,ny,nz, cr,cg,cb});
                verts.push_back({x1,y1,z1, nx,ny,nz, cr,cg,cb});
                verts.push_back({x2,y2,z2, nx,ny,nz, cr,cg,cb});
                verts.push_back({x3,y3,z3, nx,ny,nz, cr,cg,cb});
                indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
                indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
            };

            // Inner face — normal toward center (−radial), visible from inside yurt
            emitFace(b_ir_x, bY, b_ir_z,
                     b_il_x, bY, b_il_z,
                     t_il_x, uY, t_il_z,
                     t_ir_x, uY, t_ir_z,
                     -rX, 0.0f, -rZ);

            // Outer face — normal away from center (+radial)
            emitFace(b_ol_x, bY, b_ol_z,
                     b_or_x, bY, b_or_z,
                     t_or_x, uY, t_or_z,
                     t_ol_x, uY, t_ol_z,
                     rX, 0.0f, rZ);

            // Left face — normal = −tangential
            emitFace(b_ol_x, bY, b_ol_z,
                     b_il_x, bY, b_il_z,
                     t_il_x, uY, t_il_z,
                     t_ol_x, uY, t_ol_z,
                     -tX, 0.0f, -tZ);

            // Right face — normal = +tangential
            emitFace(b_ir_x, bY, b_ir_z,
                     b_or_x, bY, b_or_z,
                     t_or_x, uY, t_or_z,
                     t_ir_x, uY, t_ir_z,
                     tX, 0.0f, tZ);
        }
    }
};

#endif // YURTBEAMS_H