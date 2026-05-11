//
// Created by Erik Jourgensen on 5/11/26.
//

#include "perlinCave.h"
#include <limits>
#include <valarray>

perlinCave::perlinCave() {}
perlinCave::~perlinCave() {}

float perlinCave::dotGridGradient(const int ix, const int iy, const float x, const float y)
{
    vector2 gradient = randomGradient(ix, iy);
    const float dx = x - static_cast<float>(ix);
    const float dy = y - static_cast<float>(iy);

    return (dx * gradient.x + dy * gradient.y);
}

vector2 perlinCave::randomGradient(const int ix, const int iy)
{
    constexpr unsigned BITS     = 8 * sizeof(unsigned);
    constexpr unsigned HALF     = BITS / 2;
    constexpr float    TWO_PI   = 3.14159265f * 2.0f;

    // Hash the grid coordinates into a pseudo-random unsigned value
    unsigned a = static_cast<unsigned>(ix);
    unsigned b = static_cast<unsigned>(iy);

    auto rotl = [&](unsigned x, unsigned n) { return (x << n) | (x >> (BITS - n)); };

    a *= 3284157443u;
    b ^= rotl(a, HALF);
    b *= 1911520717u;
    a ^= rotl(b, HALF);
    a *= 2048419325u;

    // Map to [0, 2π) and return a unit vector at that angle
    const float angle = static_cast<float>(a) / static_cast<float>(std::numeric_limits<unsigned>::max()) * TWO_PI;

    return { std::cos(angle), std::sin(angle) };
}

float perlinCave::interpolate(const float a0, const float a1, const float w)
{
    // Minor correction: standard cubic smoothstep is (3.0 - 2.0 * w) * w * w
    return (a1 - a0) * (3.0f - w * 2.0f) * w * w + a0;
}

float perlinCave::perlin(float x, float y)
{
    const int x0 = static_cast<int>(x);
    const int y0 = static_cast<int>(y);
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;

    const float sx = x - static_cast<float>(x0);
    const float sy = y - static_cast<float>(y0);

    float n0 = dotGridGradient(x0, y0, x, y);
    float n1 = dotGridGradient(x1, y0, x, y);
    const float ix0 = interpolate(n0, n1, sx);
    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    const float ix1 = interpolate(n0, n1, sx);
    const float value = interpolate(ix0, ix1, sy);
    return value;
}

Vec3 perlinCave::lerpColor(Vec3 a, Vec3 b, float t)
{
    return { a.r + (b.r-a.r)*t, a.g + (b.g-a.g)*t, a.b + (b.b-a.b)*t };
}

void perlinCave::buildCaveGeometry(std::vector<Vertex>& verts, std::vector<Index>& indices)
{
    const int N = 8;
    static constexpr float PI = 3.14159265358979323846f;

    // More rings = ridges have more depth to run along
    const RingDef rings[] = {
        { -0.10f, 0.85f, 0.55f, {0.58f,0.52f,0.45f}, {0.72f,0.66f,0.58f} }, // P
        { +0.05f, 0.72f, 0.48f, {0.52f,0.47f,0.42f}, {0.65f,0.59f,0.52f} }, // P→Q
        { +0.20f, 0.55f, 0.38f, {0.42f,0.40f,0.38f}, {0.50f,0.48f,0.46f} }, // Q
        { +0.33f, 0.38f, 0.27f, {0.32f,0.31f,0.32f}, {0.38f,0.37f,0.36f} }, // Q→R
        { +0.45f, 0.25f, 0.18f, {0.22f,0.22f,0.26f}, {0.16f,0.16f,0.20f} }, // R
    };
    const int R = static_cast<int>(sizeof(rings) / sizeof(rings[0]));

    auto addRing = [&](const RingDef& ring) {
        const float floorY   = -ring.scaleY;
        const float archTopY =  ring.scaleY;
        const float avgScale = (ring.scaleX + ring.scaleY) * 0.5f;

        Vec3 c0 = lerpColor(ring.colorBottom, ring.colorTop, 0.0f);
        verts.push_back({-ring.scaleX, floorY, ring.depth, 0.f, 1.f, 0.f, c0.r, c0.g, c0.b});
        verts.push_back({+ring.scaleX, floorY, ring.depth, 0.f, 1.f, 0.f, c0.r, c0.g, c0.b});

        for (int i = 0; i < N-2; i++) {
            float t   = static_cast<float>(i + 1) / static_cast<float>(N - 1);
            float ang = t * PI;

            float x   = ring.scaleX * std::cos(ang);
            float y   = floorY + (archTopY - floorY) * std::sin(ang);

            // --------------------------------------------------------------
            // UPGRADE: 2D Perlin Noise Mapping
            // Mapping (Angle, Depth) into the Perlin noise function creates
            // organic, non-linear ridges that shift along the Z-axis.
            // --------------------------------------------------------------
            float perlinNoise = perlin(ang * 1.5f, ring.depth * 2.0f) * 0.50f
                              + perlin(ang * 4.0f, ring.depth * 3.5f) * 0.30f
                              + perlin(ang * 9.0f, ring.depth * 6.0f) * 0.20f;

            float disp = perlinNoise * avgScale * 0.35f;
            float len0 = std::sqrt(x*x + y*y);
            x += disp * (-x / len0);
            y += disp * (-y / len0);

            // Z: slightly displaced using 2D noise for a rugged floor/wall edge
            float zOff = perlin(ang * 3.0f, ring.depth * 4.0f) * avgScale * 0.06f;
            float z    = ring.depth + zOff;

            float len = std::sqrt(x*x + y*y);
            float cT  = (y - floorY) / (archTopY - floorY);
            cT        = cT < 0.f ? 0.f : (cT > 1.f ? 1.f : cT);
            Vec3 c    = lerpColor(ring.colorBottom, ring.colorTop, cT);

            float darkness = disp < 0.f ? 1.f + disp * 1.5f : 1.f;
            darkness       = darkness < 0.6f ? 0.6f : darkness;
            c.r *= darkness; c.g *= darkness; c.b *= darkness;

            verts.push_back({x, y, z, -x/len, -y/len, 0.f, c.r, c.g, c.b});
        }
    };

    for (const auto& ring : rings)
        addRing(ring);

    Index voidIdx = static_cast<Index>(verts.size());
    verts.push_back({0.f, 0.f, 0.50f, 0.f, 0.f, -1.f, 0.12f, 0.12f, 0.16f});

    auto addQuadStrip = [&](int ringA, int ringB) {
        for (int i = 0; i < N; i++) {
            int next = (i + 1) % N;
            Index a0 = static_cast<Index>(ringA*N + i);
            Index a1 = static_cast<Index>(ringA*N + next);
            Index b0 = static_cast<Index>(ringB*N + i);
            Index b1 = static_cast<Index>(ringB*N + next);
            indices.insert(indices.end(), {a0,a1,b0, a1,b1,b0});
        }
    };

    for (int i = 0; i < R-1; i++)
        addQuadStrip(i, i+1);

    // Back wall fan off last ring
    int lastRing = (R-1) * N;
    for (int i = 0; i < N; i++) {
        int next = (i + 1) % N;
        indices.insert(indices.end(), {
            voidIdx,
            static_cast<Index>(lastRing + i),
            static_cast<Index>(lastRing + next)
        });
    }
}