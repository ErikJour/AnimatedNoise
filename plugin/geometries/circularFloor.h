#ifndef CIRCULARFLOOR_H
#define CIRCULARFLOOR_H

#include <vector>
#include <cstdint>
#include <cmath>

struct FloorVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};

using FloorIndex = uint16_t;

constexpr float PI = 3.14159265358979323846f;


class CircularFloor
{
public:
    static void buildCircle(std::vector<FloorVertex>& verts,
                            std::vector<FloorIndex>&  indices,
                            const float radius   = 1.0f,
                            int   segments = 32)
    {
        verts.clear();
        indices.clear();

        if (segments < 3) segments = 3;

        verts.reserve(1 + static_cast<size_t>(segments));
        indices.reserve(static_cast<size_t>(segments) * 3);
        constexpr float floorHeight = -0.5f;

        verts.push_back({
            0.0f, floorHeight, 0.0f,
            0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f
        });

        // 2. Outer ring vertices
        for (int s = 0; s < segments; ++s)
        {
            float theta = (static_cast<float>(s) / static_cast<float>(segments)) * 2.0f * PI;

            const float px = std::cos(theta) * radius;
            const float pz = std::sin(theta) * radius;

            verts.push_back({
                px,   floorHeight, pz,
                0.0f, 1.0f, 0.0f,
                1.0f, 1.0f, 1.0f
            });
        }

        for (int s = 0; s < segments; ++s)
        {
            const int next_s = (s + 1) % segments;

            indices.push_back(0);
            indices.push_back(static_cast<FloorIndex>(1 + s));
            indices.push_back(static_cast<FloorIndex>(1 + next_s));
        }
    }
};

#endif // CIRCULARFLOOR_H