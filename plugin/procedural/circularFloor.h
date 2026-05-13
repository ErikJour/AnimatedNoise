#ifndef CIRCULARFLOOR_H
#define CIRCULARFLOOR_H

#include <vector>
#include <cstdint>
#include <cmath>

// Vertex layout: position (xyz), normal (xyz), color (rgb)
struct FloorVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};

using FloorIndex = uint16_t;

class CircularFloor
{
public:
    // Generates a simple, flat circular mesh using a triangle fan.
    static void buildCircle(std::vector<FloorVertex>& verts,
                            std::vector<FloorIndex>&  indices,
                            const float radius   = 1.0f,
                            int   segments = 32)
    {
        verts.clear();
        indices.clear();

        // A circle needs at least 3 segments (a triangle)
        if (segments < 3) segments = 3;

        verts.reserve(1 + static_cast<size_t>(segments));
        indices.reserve(static_cast<size_t>(segments) * 3);

        const float PI = 3.14159265358979323846f;

        // 1. Center vertex (Index 0)
        verts.push_back({
            0.0f, 0.0f, 0.0f, // position
            0.0f, 1.0f, 0.0f, // normal (+Y, facing up)
            1.0f, 1.0f, 1.0f
        });

        // 2. Outer ring vertices
        for (int s = 0; s < segments; ++s)
        {
            float theta = (static_cast<float>(s) / static_cast<float>(segments)) * 2.0f * PI;

            const float px = std::cos(theta) * radius;
            const float pz = std::sin(theta) * radius; // ← renamed from py

            verts.push_back({
                px,   0.0f, pz,   // ← y=0, z gets the sin value
                0.0f, 1.0f, 0.0f, // ← normal +Y
                1.0f, 1.0f, 1.0f
            });
        }

        // 3. Indices (Triangle Fan)
        for (int s = 0; s < segments; ++s)
        {
            // Wrap the last segment back to the first segment (index 1)
            const int next_s = (s + 1) % segments;

            // CCW Winding: Center (0) -> Current Outer -> Next Outer
            indices.push_back(0);
            indices.push_back(static_cast<FloorIndex>(1 + s));
            indices.push_back(static_cast<FloorIndex>(1 + next_s));
        }
    }
};

#endif // CIRCULARFLOOR_H