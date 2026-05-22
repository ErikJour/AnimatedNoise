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
        float beamHalfDepth  = 0.015f,
        int   stepsAlong     = 8,      // subdivisions along beam length
        float bulge          = 0.05f)  // max outward bow at midpoint
    {
        verts.clear();
        indices.clear();
        if (segments < 3)  segments  = 3;
        if (stepsAlong < 1) stepsAlong = 1;

        verts.reserve(static_cast<size_t>(segments) * static_cast<size_t>(stepsAlong) * 16u);
        indices.reserve(static_cast<size_t>(segments) * static_cast<size_t>(stepsAlong) * 24u);

        const float PI = 3.14159265358979323846f;
        const float cr = 0.85f, cg = 0.78f, cb = 0.65f;
        const float w = beamHalfWidth, d = beamHalfDepth;

        for (int s = 0; s < segments; ++s)
        {
            const float theta = (float(s) / float(segments)) * 2.0f * PI;
            const float cosT = std::cos(theta), sinT = std::sin(theta);
            const float rX = cosT,  rZ = sinT;
            const float tX = -sinT, tZ = cosT;

            const float bX = cosT * floorRadius,    bY = floorY,    bZ = sinT * floorRadius;
            const float uX = cosT * skylightRadius, uY = skylightY, uZ = sinT * skylightRadius;

            // Curved beam axis — sine bow pushes outward in the radial direction
            struct Pt { float x, y, z; };
            std::vector<Pt> axis(static_cast<size_t>(stepsAlong) + 1);
            for (int i = 0; i <= stepsAlong; ++i)
            {
                const float t   = float(i) / float(stepsAlong);
                const float bow = bulge * std::sin(t * PI);
                axis[static_cast<size_t>(i)] = {
                    bX + t * (uX - bX) + bow * rX,
                    bY + t * (uY - bY),
                    bZ + t * (uZ - bZ) + bow * rZ
                };
            }

            // Emit quad — normal computed from cross product of first two edges
            auto emitFace = [&](
                float x0, float y0, float z0,
                float x1, float y1, float z1,
                float x2, float y2, float z2,
                float x3, float y3, float z3)
            {
                float e1x = x1-x0, e1y = y1-y0, e1z = z1-z0;
                float e2x = x2-x0, e2y = y2-y0, e2z = z2-z0;
                float nx = e1y*e2z - e1z*e2y;
                float ny = e1z*e2x - e1x*e2z;
                float nz = e1x*e2y - e1y*e2x;
                const float nl = std::sqrt(nx*nx + ny*ny + nz*nz);
                if (nl > 1e-6f) { nx/=nl; ny/=nl; nz/=nl; }

                const auto base = static_cast<BeamIndex>(verts.size());
                verts.push_back({x0,y0,z0, nx,ny,nz, cr,cg,cb});
                verts.push_back({x1,y1,z1, nx,ny,nz, cr,cg,cb});
                verts.push_back({x2,y2,z2, nx,ny,nz, cr,cg,cb});
                verts.push_back({x3,y3,z3, nx,ny,nz, cr,cg,cb});
                indices.push_back(base+0); indices.push_back(base+1); indices.push_back(base+2);
                indices.push_back(base+0); indices.push_back(base+2); indices.push_back(base+3);
            };

            for (int i = 0; i < stepsAlong; ++i)
            {
                const auto& a0 = axis[static_cast<size_t>(i)];
                const auto& a1 = axis[static_cast<size_t>(i) + 1u];

                float b_il_x = a0.x - w*tX - d*rX, b_il_y = a0.y, b_il_z = a0.z - w*tZ - d*rZ;
                float b_ir_x = a0.x + w*tX - d*rX, b_ir_y = a0.y, b_ir_z = a0.z + w*tZ - d*rZ;
                float b_ol_x = a0.x - w*tX + d*rX, b_ol_y = a0.y, b_ol_z = a0.z - w*tZ + d*rZ;
                float b_or_x = a0.x + w*tX + d*rX, b_or_y = a0.y, b_or_z = a0.z + w*tZ + d*rZ;

                float t_il_x = a1.x - w*tX - d*rX, t_il_y = a1.y, t_il_z = a1.z - w*tZ - d*rZ;
                float t_ir_x = a1.x + w*tX - d*rX, t_ir_y = a1.y, t_ir_z = a1.z + w*tZ - d*rZ;
                float t_ol_x = a1.x - w*tX + d*rX, t_ol_y = a1.y, t_ol_z = a1.z - w*tZ + d*rZ;
                float t_or_x = a1.x + w*tX + d*rX, t_or_y = a1.y, t_or_z = a1.z + w*tZ + d*rZ;

                // Inner face (normal ≈ −radial, toward center)
                emitFace(b_ir_x, b_ir_y, b_ir_z,
                         t_ir_x, t_ir_y, t_ir_z,
                         t_il_x, t_il_y, t_il_z,
                         b_il_x, b_il_y, b_il_z);

                // Outer face (normal ≈ +radial)
                emitFace(b_or_x, b_or_y, b_or_z,
                         b_ol_x, b_ol_y, b_ol_z,
                         t_ol_x, t_ol_y, t_ol_z,
                         t_or_x, t_or_y, t_or_z);

                // Left face
                emitFace(b_il_x, b_il_y, b_il_z,
                         t_il_x, t_il_y, t_il_z,
                         t_ol_x, t_ol_y, t_ol_z,
                         b_ol_x, b_ol_y, b_ol_z);

                // Right face
                emitFace(b_ir_x, b_ir_y, b_ir_z,
                         b_or_x, b_or_y, b_or_z,
                         t_or_x, t_or_y, t_or_z,
                         t_ir_x, t_ir_y, t_ir_z);
            }
        }
    }
};

#endif // YURTBEAMS_H