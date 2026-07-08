//
// Created by Erik Jourgensen on 7/8/26.
//

#ifndef GLYPHGEOMETRY_H
#define GLYPHGEOMETRY_H

#include <vector>
#include <cstdint>
#include <cmath>
#include "FontReader.h"
#include "FontParser.h"
#include <array>
#include <string>


struct GlyphVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};

using GlyphIndex = uint16_t;

class GlyphGeometry
{
public:
    static void buildOutline(std::vector<GlyphVertex>& verts,
                             std::vector<GlyphIndex>&  indices,
                             const GlyphData&          glyph,
                             const float penX        = 0.0f,
                             const float unitsPerEm  = 1000.0f,
                             const float lineWidth   = 0.006f)
    {

        if (glyph.xCoords.empty()) return;

        const float inv  = 1.0f / unitsPerEm;
        const float half = lineWidth * 0.5f;

        constexpr float cr = 0.55f, cg = 0.95f, cb = 0.85f;

        size_t contourStart = 0;

        for (const uint16_t contourEnd : glyph.contourEndIndices)
        {
            const size_t n = contourEnd - contourStart + 1;

            for (size_t i = 0; i < n; ++i)
            {
                const size_t a = contourStart + i;
                const size_t b = contourStart + (i + 1) % n;

                const float ax = static_cast<float>(glyph.xCoords[a]) * inv + penX;
                const float ay = static_cast<float>(glyph.yCoords[a]) * inv;
                const float bx = static_cast<float>(glyph.xCoords[b]) * inv + penX;
                const float by = static_cast<float>(glyph.yCoords[b]) * inv;

                float dx = bx - ax, dy = by - ay;
                const float len = std::sqrt(dx * dx + dy * dy);
                if (len < 1e-6f) continue;
                const float px = -dy / len * half;
                const float py =  dx / len * half;

                const auto base = static_cast<GlyphIndex>(verts.size());

                verts.push_back({ ax + px, ay + py, 0.0f,  0, 0, 1,  cr, cg, cb });
                verts.push_back({ ax - px, ay - py, 0.0f,  0, 0, 1,  cr, cg, cb });
                verts.push_back({ bx + px, by + py, 0.0f,  0, 0, 1,  cr, cg, cb });
                verts.push_back({ bx - px, by - py, 0.0f,  0, 0, 1,  cr, cg, cb });

                indices.push_back(base);
                indices.push_back(static_cast<GlyphIndex>(base + 1));
                indices.push_back(static_cast<GlyphIndex>(base + 2));
                indices.push_back(static_cast<GlyphIndex>(base + 1));
                indices.push_back(static_cast<GlyphIndex>(base + 3));
                indices.push_back(static_cast<GlyphIndex>(base + 2));
            }

            contourStart = contourEnd + 1;
        }
    }

    static void buildString(std::vector<GlyphVertex>& verts,
                            std::vector<GlyphIndex>&  indices,
                            FontParser&               font,
                            const std::string&        text,
                            const float lineWidth     = 0.012f)
    {
        verts.clear();
        indices.clear();

        const float upem = static_cast<float>(font.unitsPerEm());
        const float inv  = 1.0f / upem;
        float penX = 0.0f;

        for (const char c : text)
        {
            const uint16_t  gi = font.glyphIndexForChar(static_cast<unsigned char>(c));
            const GlyphData g  = font.getGlyphByIndex(gi);
            buildOutline(verts, indices, g, penX, upem, lineWidth);
            penX += static_cast<float>(g.advanceWidth) * inv;
        }
    }

    // scale: em-units -> scene units.  position: where the string's baseline origin sits.
    static std::array<float, 16> makeTextModel(float sx, float sy, float sz,
                                               float tx, float ty, float tz)
    {
        return {
            sx,  0,   0,   0,
            0,   sy,  0,   0,
            0,   0,   sz,  0,
            tx,  ty,  tz,  1
        };
    }
};

#endif // GLYPHGEOMETRY_H