#ifndef ANIMATEDNOISE_PROCEDURALCAVE_H
#define ANIMATEDNOISE_PROCEDURALCAVE_H
#include <vector>
#include <cmath>

struct Vertex { float x, y, z, nx, ny, nz, r, g, b; };
using Index = uint16_t;

struct Vec3 { float r, g, b; };
struct RingDef {
    float depth, scaleX, scaleY;
    Vec3 colorBottom, colorTop;
};

inline Vec3 lerpColor(Vec3 a, Vec3 b, float t) {
    return { a.r + (b.r-a.r)*t, a.g + (b.g-a.g)*t, a.b + (b.b-a.b)*t };
}

inline float ihash(int n) {
    n = (n << 13) ^ n;
    int m = (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff;
    return 1.0f - static_cast<float>(m) / 1073741824.0f;
}

inline float vnoise(float x, int seed) {
    int   xi = static_cast<int>(floorf(x));
    float xf = x - floorf(x);
    float u  = xf * xf * (3.0f - 2.0f * xf);
    return ihash(xi + seed) * (1.0f - u) + ihash(xi + 1 + seed) * u;
}

// Noise is ANGLE-ONLY — same displacement at each angle across all rings = vertical ridges
inline float rockNoise(float ang) {
    return vnoise(ang * 1.5f, 0) * 0.50f
         + vnoise(ang * 4.0f, 3) * 0.30f
         + vnoise(ang * 9.0f, 7) * 0.20f;
}

inline void buildCaveGeometry(std::vector<Vertex>& verts, std::vector<Index>& indices) {
    const int N = 16;
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

            float x   = ring.scaleX * cosf(ang);
            float y   = floorY + (archTopY - floorY) * sinf(ang);

            // Same noise for every ring at this angle → vertical ridge
            float disp = rockNoise(ang) * avgScale * 0.35f;
            float len0 = sqrtf(x*x + y*y);
            x += disp * (-x / len0);
            y += disp * (-y / len0);

            // Z: consistent per angle across rings (deepens the ridge channels)
            float zOff = vnoise(ang * 3.0f, 5) * avgScale * 0.06f;
            float z    = ring.depth + zOff;

            float len = sqrtf(x*x + y*y);
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

#endif