// perlinPlane.h
// Created by Erik Jourgensen on 5/11/26.

#ifndef ANIMATEDNOISE_PERLINPLANE_H
#define ANIMATEDNOISE_PERLINPLANE_H

#include <vector>
#include <cstdint>
#include <cmath>
#include <cstdlib>

// Vertex layout: position (xyz), normal (xyz), color (rgb)
// @location(0) position : vec3f   -- offset  0, stride 36
// @location(2) normal   : vec3f   -- offset 12
// @location(1) color    : vec3f   -- offset 24
struct PlaneVertex {
    float x, y, z;     // position
    float nx, ny, nz;  // normal  (always 0,0,1 for a flat plane)
    float r, g, b;        // uv      (0..1 across width/height)
};

using PlaneIndex = uint16_t;

class Plane
{
public:
    // Equivalent to: new THREE.PlaneGeometry(width, height, widthSegs, heightSegs)
    // Plane lies in XY, normal points +Z.
    static void buildPlane(std::vector<PlaneVertex>& verts,
                      std::vector<PlaneIndex>&  indices,
                      float width       = 1.0f,
                      float height      = 1.0f,
                      int   widthSegs   = 32,
                      int   heightSegs  = 32)
    {
        verts.clear();
        indices.clear();

        const int cols = widthSegs + 1;
        const int rows = heightSegs + 1;

        verts.reserve  (static_cast<size_t>(cols * rows));
        indices.reserve(static_cast<size_t>(widthSegs * heightSegs * 6));

        for (int row = 0; row < rows; ++row)
        {
            const float t  = static_cast<float>(row) / static_cast<float>(heightSegs);
            const float py = (t - 0.5f) * height;

            for (int col = 0; col < cols; ++col)
            {
                const float s  = static_cast<float>(col) / static_cast<float>(widthSegs);
                const float px = (s - 0.5f) * width;

                PlaneVertex vert{};
                vert.x  = px;   vert.y  = py;   vert.z  = 0.0f;
                vert.nx = 0.0f; vert.ny = 0.0f; vert.nz = 1.0f;
                vert.r  = 0.25f;    vert.g  = 0.25f;  // flip V to match UV convention
                vert.b = 0.3f;

                verts.push_back(vert);
            }
        }

        for (int row = 0; row < heightSegs; ++row)
        {
            for (int col = 0; col < widthSegs; ++col)
            {
                const auto tl = static_cast<PlaneIndex>( row      * cols + col);
                const auto tr = static_cast<PlaneIndex>( row      * cols + col + 1);
                const auto bl = static_cast<PlaneIndex>((row + 1) * cols + col);
                const auto br = static_cast<PlaneIndex>((row + 1) * cols + col + 1);

                // Two CCW triangles per quad
                indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
                indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
            }
        }
    }
};

#endif // ANIMATEDNOISE_PERLINPLANE_H