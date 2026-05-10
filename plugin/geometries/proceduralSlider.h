#ifndef ANIMATEDNOISE_PROCEDURALSLIDER_H
#define ANIMATEDNOISE_PROCEDURALSLIDER_H
#include <vector>
#include <cmath>
// Vertex, Index, Vec3 defined in proceduralCave.h

inline void buildSliderGeometry(std::vector<Vertex>& verts, std::vector<Index>& indices) {
    static constexpr float PI = 3.14159265358979323846f;

    const Vec3 spineColor     = {0.28f, 0.14f, 0.07f};  // sentinel
    const Vec3 indicatorColor = {1.00f, 0.40f, 0.10f};  // sentinel

    const float yBottom = -0.150f;
    const float yTop    =  0.250f;
    const float yRange  = yTop - yBottom;

    // ── Tendril spine ─────────────────────────────────────────────────────────
    const int N = 14;  // levels along y

    for (int i = 0; i < N; i++) {
        float t  = static_cast<float>(i) / static_cast<float>(N - 1);
        float y  = yBottom + t * yRange;

        // Width: parabolic taper — pinched at tips, widest at mid
        float tBiased = t * 0.5f;  // peak at ~35% up

        float w = 0.002f + 0.010f * (4.0f * tBiased * (1.0f - tBiased));

        // S-curve: drifts right through lower half, left through upper
        float cx = 0.007f * sinf(PI * t * 1.5f);

        verts.push_back({cx - w, y, 0.f, 0.f, 0.f, -1.f, spineColor.r, spineColor.g, spineColor.b});
        verts.push_back({cx + w, y, 0.f, 0.f, 0.f, -1.f, spineColor.r, spineColor.g, spineColor.b});
    }

    for (int i = 0; i < N - 1; i++) {
        Index bl = static_cast<Index>(2 * i);
        Index br = static_cast<Index>(2 * i + 1);
        Index tl = static_cast<Index>(2 * i + 2);
        Index tr = static_cast<Index>(2 * i + 3);
        indices.insert(indices.end(), {bl, br, tl, br, tr, tl});
    }

    // ── Indicator — oval bud, fan from center ─────────────────────────────────
    const int   N_IND = 12;
    const float rx    = 0.022f;
    const float ry    = 0.014f;

    Index centerIdx = static_cast<Index>(verts.size());
    verts.push_back({0.f, 0.f, -0.005f, 0.f, 0.f, -1.f,
                     indicatorColor.r, indicatorColor.g, indicatorColor.b});

    Index rimStart = static_cast<Index>(verts.size());
    for (int i = 0; i < N_IND; i++) {
        float ang = 2.0f * PI * static_cast<float>(i) / static_cast<float>(N_IND);
        verts.push_back({rx * cosf(ang), ry * sinf(ang), -0.005f, 0.f, 0.f, -1.f,
                         indicatorColor.r, indicatorColor.g, indicatorColor.b});
    }

    for (int i = 0; i < N_IND; i++) {
        int next = (i + 1) % N_IND;
        indices.insert(indices.end(), {
            centerIdx,
            static_cast<Index>(rimStart + i),
            static_cast<Index>(rimStart + next)
        });
    }
}

#endif