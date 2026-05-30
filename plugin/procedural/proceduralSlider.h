#ifndef ANIMATEDNOISE_PROCEDURALSLIDER_H
#define ANIMATEDNOISE_PROCEDURALSLIDER_H
#include <vector>
#include <cmath>

struct SliderVertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v, pad;
};
using SliderIndex = uint16_t;

inline void buildSliderGeometry(
    std::vector<SliderVertex>& verts,
    std::vector<SliderIndex>&  indices,
    const float wallRadius  = 0.9f,
    const float centerAngle = 0.0f,
    const float halfSpan    = 0.25f,
    const float yBottom     = 0.15f,
    const float yTop        =  0.25f,
    const int   nArc        = 10,
    const int   nY          = 40)
{
    for (int j = 0; j <= nY; ++j)
    {
        constexpr float kPI = 3.14159265f;
        const float v = static_cast<float>(j) / static_cast<float>(nY);
        const float y = yBottom + v * (yTop - yBottom);

        // ── Leaf / spinal-fin silhouette ──────────────────────────────────────
        const float leafBulge  = std::sinf(v * kPI);
        const float localSpan  = halfSpan * (0.02f + 0.28f * leafBulge);

        for (int i = 0; i <= nArc; ++i)
        {
            const float u     = static_cast<float>(i) / static_cast<float>(nArc);
            const float theta = centerAngle - localSpan + u * 2.0f * localSpan;
            const float cosT  = std::cosf(theta);
            const float sinT  = std::sinf(theta);

            SliderVertex sv{};
            sv.x   =  wallRadius * cosT;
            sv.y   =  y;
            sv.z   =  wallRadius * sinT;
            sv.nx  = -cosT;
            sv.ny  =  0.0f;
            sv.nz  = -sinT;
            sv.u   =  u;
            sv.v   =  v;
            sv.pad =  0.0f;
            verts.push_back(sv);
        }
    }

    const int row = nArc + 1;
    for (int j = 0; j < nY; ++j)
        for (int i = 0; i < nArc; ++i)
        {
            const auto bl = static_cast<SliderIndex>( j      * row + i);
            const auto br = static_cast<SliderIndex>( j      * row + i + 1);
            const auto tl = static_cast<SliderIndex>((j + 1) * row + i);
            const auto tr = static_cast<SliderIndex>((j + 1) * row + i + 1);
            indices.insert(indices.end(), {bl, br, tl, br, tr, tl});
        }
}

#endif