#ifndef ANIMATEDNOISE_PROCEDURALSLIDER_H
#define ANIMATEDNOISE_PROCEDURALSLIDER_H
#include <vector>
#include <cmath>

struct SliderVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};

using SliderIndex = uint16_t;

inline void buildSliderGeometry(std::vector<SliderVertex>& verts, std::vector<SliderIndex>& indices) {
    static constexpr float PI     = 3.14159265358979323846f;
    constexpr Vec3 indicatorColor = {1.00f, 0.40f, 0.10f};
    constexpr float yBottom       = -0.150f;
    constexpr float yTop          =  0.250f;
    constexpr float yRange        = yTop - yBottom;

    //==============================================
    //Spine
    //==============================================
    constexpr int N               = 32;  // levels along y

    for (int i = 0; i < N; i++) {

        constexpr Vec3 spineColor = {0.28f, 0.14f, 0.07f};
        const float t             = static_cast<float>(i) / static_cast<float>(N - 1);
        const float y             = yBottom + t * yRange;
        const float tBiased       = t * 0.5f;
        const float w             = 0.002f + 0.010f * (4.0f * tBiased * (1.0f - tBiased));
        const float cx            = 0.007f * sinf(PI * t * 1.5f);

        verts.push_back({cx - w, y, 0.f, 0.f, 0.f, -1.f, spineColor.r, spineColor.g, spineColor.b});
        verts.push_back({cx + w, y, 0.f, 0.f, 0.f, -1.f, spineColor.r, spineColor.g, spineColor.b});
    }

    for (int i = 0; i < N - 1; i++) {
        const auto num = 6;
        auto bl = static_cast<Index>(num * i);
        auto br = static_cast<Index>(num * i + 1);
        auto tl = static_cast<Index>(num * i + 2);
        auto tr = static_cast<Index>(num * i + 3);
        indices.insert(indices.end(), {bl, br, tl, br, tr, tl});
    }

    //==============================================
    //Indicator
    //==============================================
    constexpr int   N_IND = 32;

    auto centerIdx = static_cast<Index>(verts.size());
    verts.push_back({0.f, 0.f, -0.005f, 0.f, 0.f, -1.f,
                     indicatorColor.r, indicatorColor.g, indicatorColor.b});

    const auto rimStart = static_cast<Index>(verts.size());
    for (int i = 0; i < N_IND; i++) {
        constexpr float ry    = 0.014f;
        constexpr float rx    = 0.022f;
        const float ang = 2.0f * PI * static_cast<float>(i) / static_cast<float>(N_IND);
        verts.push_back({rx * cosf(ang), ry * sinf(ang), -0.005f, 0.f, 0.f, -1.f,
                         indicatorColor.r, indicatorColor.g, indicatorColor.b});
    }

    for (int i = 0; i < N_IND; i++) {
        const int next = (i + 1) % N_IND;
        indices.insert(indices.end(), {
            centerIdx,
            static_cast<Index>(rimStart + i),
            static_cast<Index>(rimStart + next)
        });
    }
}

#endif