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
#include <algorithm>
#include <numeric>
#include <cameraState.h>


struct GlyphVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};

using GlyphIndex = uint16_t;

enum class GlyphStyle { Outline, Filled };

class GlyphGeometry
{
public:
    static vec2 linearInterpolation(const vec2 start, const vec2 end, const float time)
    {
        return start + (end - start) * time;
    }

    static vec2 bezierInterpolation( vec2 p0, vec2 p1, vec2 p2, float time)
    {
        const vec2 intermediateA = linearInterpolation(p0, p1, time);
        const vec2 intermediateB = linearInterpolation(p1, p2, time);
        return linearInterpolation(intermediateA, intermediateB, time);
    }

    static void drawLine(std::vector<GlyphVertex>& verts,
                     std::vector<GlyphIndex>&  indices,
                     const vec2 a, const vec2 b,
                     const float half,                  // lineWidth * 0.5f
                     const float cr = 1.0f, const float cg = 1.0f, const float cb = 1.0f)
    {
        const float dx = b.x - a.x, dy = b.y - a.y;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-6f) return;

        const float px = -dy / len * half;
        const float py =  dx / len * half;

        const auto base = static_cast<GlyphIndex>(verts.size());

        verts.push_back({ a.x + px, a.y + py, 0.0f,  0, 0, 1,  cr, cg, cb });
        verts.push_back({ a.x - px, a.y - py, 0.0f,  0, 0, 1,  cr, cg, cb });
        verts.push_back({ b.x + px, b.y + py, 0.0f,  0, 0, 1,  cr, cg, cb });
        verts.push_back({ b.x - px, b.y - py, 0.0f,  0, 0, 1,  cr, cg, cb });

        indices.push_back(base);
        indices.push_back(static_cast<GlyphIndex>(base + 1));
        indices.push_back(static_cast<GlyphIndex>(base + 2));
        indices.push_back(static_cast<GlyphIndex>(base + 1));
        indices.push_back(static_cast<GlyphIndex>(base + 3));
        indices.push_back(static_cast<GlyphIndex>(base + 2));
    }

    //--- contour extraction -----------------------------------------------------------------
    // A closed polyline, in font units. The last point does not repeat the first.
    using Contour = std::vector<vec2>;

    // Tolerance for merging coincident points, in font units (em is typically 1000).
    static constexpr float kMergeEps = 1e-3f;

    static bool nearlyEqual(const vec2 a, const vec2 b)
    {
        return std::fabs(a.x - b.x) < kMergeEps && std::fabs(a.y - b.y) < kMergeEps;
    }

    // Walks a glyph's contours, flattening quadratic curves into line segments.
    // Curves are stored as on-curve points separated by off-curve control points; two
    // adjacent control points imply an on-curve point at their midpoint.
    static std::vector<Contour> flattenGlyph(const GlyphData& glyph, const int resolution = 8)
    {
        std::vector<Contour> contours;
        if (glyph.xCoords.empty()) return contours;

        size_t contourStart = 0;

        for (const uint16_t contourEnd : glyph.contourEndIndices)
        {
            const size_t n = contourEnd - contourStart + 1;
            if (n < 2) { contourStart = contourEnd + 1; continue; }

            auto point = [&](const size_t k) {
                const size_t idx = contourStart + k % n;
                return vec2 { static_cast<float>(glyph.xCoords[idx]),
                              static_cast<float>(glyph.yCoords[idx]) };
            };
            auto isOnCurve = [&](const size_t k) { return glyph.onCurve[contourStart + k % n]; };

            // Walk from an on-curve point. A contour with none is all control points,
            // and the outline passes through the implied midpoints between them.
            size_t start = 0;
            while (start < n && !isOnCurve(start))
                ++start;

            vec2 cursor;
            if (start == n)
            {
                start  = n - 1;
                cursor = linearInterpolation(point(n - 1), point(0), 0.5f);
            }
            else
            {
                cursor = point(start);
            }

            Contour c;
            c.push_back(cursor);

            const size_t last = start + n;

            for (size_t i = start; i < last; )
            {
                if (isOnCurve(i + 1))
                {
                    cursor = point(i + 1);
                    c.push_back(cursor);
                    ++i;
                }
                else
                {
                    // i + 1 is the control point. The curve ends on the point after it,
                    // or on the implied midpoint if that one is a control point too.
                    const vec2 control     = point(i + 1);
                    const bool endsOnPoint = isOnCurve(i + 2);

                    const vec2 endPt = endsOnPoint
                                     ? point(i + 2)
                                     : linearInterpolation(control, point(i + 2), 0.5f);

                    for (int s = 1; s <= resolution; ++s)
                        c.push_back(bezierInterpolation(cursor, control, endPt,
                                        static_cast<float>(s) / static_cast<float>(resolution)));

                    cursor = endPt;
                    i += endsOnPoint ? 2 : 1;
                }
            }

            // The walk closes back onto its own start; drop coincident points.
            Contour cleaned;
            for (const vec2 p : c)
                if (cleaned.empty() || !nearlyEqual(cleaned.back(), p)) cleaned.push_back(p);
            while (cleaned.size() > 1 && nearlyEqual(cleaned.front(), cleaned.back()))
                cleaned.pop_back();

            if (cleaned.size() >= 3) contours.push_back(std::move(cleaned));

            contourStart = contourEnd + 1;
        }

        return contours;
    }

    static void buildOutline(std::vector<GlyphVertex>& verts,
                             std::vector<GlyphIndex>&  indices,
                             const GlyphData&          glyph,
                             const float penX        = 0.0f,
                             const float unitsPerEm  = 1000.0f,
                             const float lineWidth   = 0.006f,
                             const int   resolution  = 8)
    {
        const float inv  = 1.0f / unitsPerEm;
        const float half = lineWidth * 0.5f;

        for (const Contour& c : flattenGlyph(glyph, resolution))
        {
            auto scaled = [&](const vec2 p) { return vec2 { p.x * inv + penX, p.y * inv }; };

            for (size_t i = 0; i < c.size(); ++i)
                drawLine(verts, indices, scaled(c[i]), scaled(c[(i + 1) % c.size()]), half);
        }
    }

    //--- planar predicates ------------------------------------------------------------------
    // > 0 when o -> a -> b turns counter-clockwise.
    static float orient(const vec2 o, const vec2 a, const vec2 b)
    {
        return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
    }

    static float signedArea(const Contour& c)
    {
        float a = 0.0f;
        for (size_t i = 0, j = c.size() - 1; i < c.size(); j = i++)
            a += c[j].x * c[i].y - c[i].x * c[j].y;
        return a * 0.5f;
    }

    static bool pointInPolygon(const vec2 p, const Contour& c)
    {
        bool inside = false;
        for (size_t i = 0, j = c.size() - 1; i < c.size(); j = i++)
            if ((c[i].y > p.y) != (c[j].y > p.y) &&
                p.x < (c[j].x - c[i].x) * (p.y - c[i].y) / (c[j].y - c[i].y) + c[i].x)
                inside = !inside;
        return inside;
    }

    // Closed triangle, abc counter-clockwise: points on an edge count as inside. An ear whose
    // diagonal merely grazes a reflex vertex leaves a self-touching ring, so grazes must block.
    static bool pointInTriangle(const vec2 a, const vec2 b, const vec2 c, const vec2 p)
    {
        constexpr float e = 1e-6f;
        return orient(a, b, p) >= -e && orient(b, c, p) >= -e && orient(c, a, p) >= -e;
    }

    // True only when the segment interiors properly cross. A zero orientation means an
    // endpoint is collinear with the other segment — a touch, not a crossing. Bridges are
    // anchored on existing vertices, so touches are the common case and must not count.
    static bool segmentsCross(const vec2 a, const vec2 b, const vec2 c, const vec2 d)
    {
        constexpr float kOrientEps = 1e-3f;   // font units squared

        const float d1 = orient(c, d, a), d2 = orient(c, d, b);
        const float d3 = orient(a, b, c), d4 = orient(a, b, d);

        if (std::fabs(d1) < kOrientEps || std::fabs(d2) < kOrientEps ||
            std::fabs(d3) < kOrientEps || std::fabs(d4) < kOrientEps)
            return false;

        return ((d1 > 0.0f) != (d2 > 0.0f)) && ((d3 > 0.0f) != (d4 > 0.0f));
    }

    // p lies on the open segment (a, b), endpoints excluded.
    static bool pointOnSegment(const vec2 a, const vec2 b, const vec2 p)
    {
        if (nearlyEqual(p, a) || nearlyEqual(p, b)) return false;

        const float dx = b.x - a.x, dy = b.y - a.y;
        const float len2 = dx * dx + dy * dy;
        if (len2 <= 0.0f) return false;

        const float t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
        if (t <= 0.0f || t >= 1.0f) return false;

        const float qx = a.x + t * dx - p.x, qy = a.y + t * dy - p.y;
        return qx * qx + qy * qy < kMergeEps * kMergeEps;
    }

    //--- self-intersection ------------------------------------------------------------------
    // Where the segment interiors properly cross, with the parameter along each.
    static bool segmentIntersection(const vec2 a, const vec2 b, const vec2 c, const vec2 d,
                                    float& t, float& u, vec2& p)
    {
        const vec2 r { b.x - a.x, b.y - a.y };
        const vec2 s { d.x - c.x, d.y - c.y };

        const float denom = r.x * s.y - r.y * s.x;
        if (std::fabs(denom) < 1e-9f) return false;             // parallel or collinear

        const vec2 ac { c.x - a.x, c.y - a.y };
        t = (ac.x * s.y - ac.y * s.x) / denom;
        u = (ac.x * r.y - ac.y * r.x) / denom;

        constexpr float e = 1e-6f;
        if (t <= e || t >= 1.0f - e || u <= e || u >= 1.0f - e) return false;

        p = vec2 { a.x + r.x * t, a.y + r.y * t };
        return true;
    }

    // Outlines legitimately cross themselves where strokes meet — the diagonal of 'Z' runs into
    // its top bar, leaving a small fold that the nonzero rule still fills. Ear clipping needs a
    // simple polygon, so cut the contour into simple loops at its crossings.
    static std::vector<Contour> splitSelfIntersections(const Contour& c)
    {
        const size_t n = c.size();
        if (n < 4) return { c };

        struct Cut { float t; uint32_t id; vec2 p; };
        std::vector<std::vector<Cut>> cuts(n);
        uint32_t nextId = static_cast<uint32_t>(n);

        for (size_t i = 0; i < n; ++i)
            for (size_t j = i + 1; j < n; ++j)
            {
                if (j == i + 1 || (i == 0 && j == n - 1)) continue;   // edges that share a vertex

                float t = 0.0f, u = 0.0f; vec2 p{};
                if (!segmentIntersection(c[i], c[(i + 1) % n], c[j], c[(j + 1) % n], t, u, p)) continue;

                const uint32_t id = nextId++;   // one id, shared by both edges
                cuts[i].push_back({ t, id, p });
                cuts[j].push_back({ u, id, p });
            }

        bool crossed = false;
        for (const auto& e : cuts) crossed = crossed || !e.empty();
        if (!crossed) return { c };

        // Walk the contour with the crossings spliced in; each crossing is visited twice.
        std::vector<std::pair<uint32_t, vec2>> walk;
        for (size_t i = 0; i < n; ++i)
        {
            walk.emplace_back(static_cast<uint32_t>(i), c[i]);
            std::sort(cuts[i].begin(), cuts[i].end(), [](const Cut& a, const Cut& b) { return a.t < b.t; });
            for (const Cut& cut : cuts[i]) walk.emplace_back(cut.id, cut.p);
        }

        // Revisiting a crossing closes a loop: pop it off and carry on from that point.
        std::vector<Contour> loops;
        std::vector<std::pair<uint32_t, vec2>> stack;

        for (const auto& step : walk)
        {
            bool   revisited = false;
            size_t at = 0;
            for (size_t m = stack.size(); m-- > 0; )
                if (stack[m].first == step.first) { at = m; revisited = true; break; }

            if (!revisited) { stack.push_back(step); continue; }

            Contour loop;
            for (size_t m = at; m < stack.size(); ++m) loop.push_back(stack[m].second);
            if (loop.size() >= 3) loops.push_back(std::move(loop));
            stack.resize(at + 1);
        }

        Contour tail;
        for (const auto& s : stack) tail.push_back(s.second);
        if (tail.size() >= 3) loops.push_back(std::move(tail));

        return loops;
    }

    //--- triangulation ----------------------------------------------------------------------
    // Splice a hole into the outer ring by doubling the bridge vertices, turning the
    // ring-with-hole into one simple polygon with a zero-width slit.
    static Contour spliceHole(const Contour& poly, const size_t m, const Contour& hole, const size_t h)
    {
        Contour out;
        out.reserve(poly.size() + hole.size() + 2);

        for (size_t i = 0; i <= m; ++i)             out.push_back(poly[i]);
        for (size_t k = 0; k < hole.size(); ++k)    out.push_back(hole[(h + k) % hole.size()]);
        out.push_back(hole[h]);
        for (size_t i = m; i < poly.size(); ++i)    out.push_back(poly[i]);

        return out;
    }

    // A bridge is usable when it stays strictly inside the ring, clears every hole, and
    // crosses no existing edge.
    static bool bridgeIsValid(const Contour& poly, const Contour& outer,
                              const std::vector<Contour>& holes, const size_t firstUnmerged,
                              const vec2 h, const vec2 m)
    {
        if (nearlyEqual(h, m)) return false;

        const vec2 mid { (h.x + m.x) * 0.5f, (h.y + m.y) * 0.5f };
        if (!pointInPolygon(mid, outer)) return false;
        for (const Contour& hole : holes)
            if (pointInPolygon(mid, hole)) return false;

        // Touches are not crossings, so a bridge running through a vertex must be rejected here.
        auto blockedBy = [&](const Contour& c) {
            for (size_t i = 0; i < c.size(); ++i)
            {
                if (segmentsCross(h, m, c[i], c[(i + 1) % c.size()])) return true;
                if (pointOnSegment(h, m, c[i])) return true;
            }
            return false;
        };

        if (blockedBy(poly)) return false;

        // Holes already merged live in poly; the rest are still standalone.
        for (size_t j = firstUnmerged; j < holes.size(); ++j)
            if (blockedBy(holes[j])) return false;

        return true;
    }

    static Contour bridgeHoles(const Contour& outer, std::vector<Contour> holes)
    {
        if (holes.empty()) return outer;

        auto leftmost = [](const Contour& c) {
            size_t k = 0;
            for (size_t i = 1; i < c.size(); ++i)
                if (c[i].x < c[k].x ||

                    (std::fabs(c[i].x - c[k].x) < kMergeEps && c[i].y < c[k].y)) k = i;
            return k;
        };
        auto dist2 = [](const vec2 a, const vec2 b) {
            const float dx = a.x - b.x, dy = a.y - b.y;
            return dx * dx + dy * dy;
        };

        std::sort(holes.begin(), holes.end(), [&](const Contour& a, const Contour& b) {
            return a[leftmost(a)].x < b[leftmost(b)].x;
        });

        Contour poly = outer;

        for (size_t hi = 0; hi < holes.size(); ++hi)
        {
            const Contour& hole = holes[hi];
            const size_t   l    = leftmost(hole);
            bool merged = false;

            for (size_t s = 0; s < hole.size() && !merged; ++s)
            {
                const size_t h = (l + s) % hole.size();

                std::vector<size_t> candidates(poly.size());
                std::iota(candidates.begin(), candidates.end(), size_t{0});
                std::sort(candidates.begin(), candidates.end(), [&](size_t a, size_t b) {
                    return dist2(poly[a], hole[h]) < dist2(poly[b], hole[h]);
                });

                for (const size_t m : candidates)
                {
                    if (!bridgeIsValid(poly, outer, holes, hi, hole[h], poly[m])) continue;
                    poly   = spliceHole(poly, m, hole, h);
                    merged = true;
                    break;
                }
            }
        }

        return poly;
    }

    // Ear clipping. The polygon must be simple; holes are bridged in beforehand.
    static void earClip(const Contour& poly, std::vector<uint32_t>& tris)
    {
        const size_t n = poly.size();
        if (n < 3) return;

        std::vector<uint32_t> ring(n);
        std::iota(ring.begin(), ring.end(), 0u);
        if (signedArea(poly) < 0.0f) std::reverse(ring.begin(), ring.end());

        size_t guard = 0;
        const size_t maxIterations = n * n + 16;

        while (ring.size() > 3 && guard++ < maxIterations)
        {
            bool clipped = false;
            const size_t m = ring.size();

            for (size_t i = 0; i < m; ++i)
            {
                const size_t   ia = (i + m - 1) % m, ic = (i + 1) % m;
                const uint32_t a  = ring[ia], b = ring[i], c = ring[ic];

                if (orient(poly[a], poly[b], poly[c]) <= 0.0f) continue;   // reflex or collinear

                // Only a reflex vertex can spoil an ear. Vertices coincident with a corner are
                // the two halves of a hole bridge, and sit on the triangle without spoiling it.
                bool blocked = false;
                for (size_t j = 0; j < m && !blocked; ++j)
                {
                    if (j == ia || j == i || j == ic) continue;

                    const vec2 p = poly[ring[j]];
                    if (nearlyEqual(p, poly[a]) || nearlyEqual(p, poly[b]) || nearlyEqual(p, poly[c]))
                        continue;

                    const vec2 prev = poly[ring[(j + m - 1) % m]];
                    const vec2 next = poly[ring[(j + 1) % m]];
                    if (orient(prev, p, next) > 0.0f) continue;            // convex, cannot block

                    blocked = pointInTriangle(poly[a], poly[b], poly[c], p);
                }
                if (blocked) continue;

                tris.push_back(a); tris.push_back(b); tris.push_back(c);
                ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(i));
                clipped = true;
                break;
            }

            if (clipped) continue;

            // No ear found: shed a collinear vertex, which carries no area, and retry.
            bool dropped = false;
            const size_t m2 = ring.size();
            for (size_t i = 0; i < m2; ++i)
            {
                const uint32_t a = ring[(i + m2 - 1) % m2], b = ring[i], c = ring[(i + 1) % m2];
                if (std::fabs(orient(poly[a], poly[b], poly[c])) < 1e-6f)
                {
                    ring.erase(ring.begin() + static_cast<std::ptrdiff_t>(i));
                    dropped = true;
                    break;
                }
            }
            if (!dropped) break;
        }

        if (ring.size() == 3) { tris.push_back(ring[0]); tris.push_back(ring[1]); tris.push_back(ring[2]); }
    }

    static void buildFilled(std::vector<GlyphVertex>& verts,
                            std::vector<GlyphIndex>&  indices,
                            const GlyphData&          glyph,
                            const float penX        = 0.0f,
                            const float unitsPerEm  = 1000.0f,
                            const int   resolution  = 8)
    {
        std::vector<Contour> contours;
        for (const Contour& raw : flattenGlyph(glyph, resolution))
            for (Contour& loop : splitSelfIntersections(raw))
                if (std::fabs(signedArea(loop)) > 1.0f)          // drop slivers, in font units
                    contours.push_back(std::move(loop));

        if (contours.empty()) return;

        std::vector<float> area(contours.size());
        for (size_t i = 0; i < contours.size(); ++i)
            area[i] = signedArea(contours[i]);

        // Glyphs fill under the nonzero winding rule, so a contour is a counter only when it
        // is wound against the outer direction. Contours wound *with* it are solid even where
        // they overlap — the two bars and the crossbar of 'H' are three overlapping rectangles
        // in this face, and nesting depth alone would mistake the crossbar for a hole.
        size_t biggest = 0;
        for (size_t i = 1; i < contours.size(); ++i)
            if (std::fabs(area[i]) > std::fabs(area[biggest])) biggest = i;

        const bool outerIsPositive = area[biggest] > 0.0f;

        std::vector<bool> isHole(contours.size());
        for (size_t i = 0; i < contours.size(); ++i)
            isHole[i] = (area[i] > 0.0f) != outerIsPositive;

        // Ear clipping wants solid rings CCW and counters CW.
        for (size_t i = 0; i < contours.size(); ++i)
            if (isHole[i] == (area[i] > 0.0f))
                std::reverse(contours[i].begin(), contours[i].end());

        // A counter belongs to the tightest solid ring enclosing it, so an island inside a
        // counter keeps its own counters.
        constexpr size_t kNone = ~size_t{0};
        std::vector<size_t> owner(contours.size(), kNone);

        for (size_t h = 0; h < contours.size(); ++h)
        {
            if (!isHole[h]) continue;
            for (size_t o = 0; o < contours.size(); ++o)
            {
                if (isHole[o] || !pointInPolygon(contours[h][0], contours[o])) continue;
                if (owner[h] == kNone || std::fabs(area[o]) < std::fabs(area[owner[h]]))
                    owner[h] = o;
            }
        }

        const float inv = 1.0f / unitsPerEm;

        for (size_t o = 0; o < contours.size(); ++o)
        {
            if (isHole[o]) continue;

            std::vector<Contour> holes;
            for (size_t h = 0; h < contours.size(); ++h)
                if (isHole[h] && owner[h] == o) holes.push_back(contours[h]);

            const Contour poly = bridgeHoles(contours[o], std::move(holes));

            std::vector<uint32_t> tris;
            earClip(poly, tris);
            if (tris.empty()) continue;

            const auto base = static_cast<GlyphIndex>(verts.size());
            for (const vec2 p : poly)
                verts.push_back({ p.x * inv + penX, p.y * inv, 0.0f,  0, 0, 1,  1, 1, 1 });
            for (const uint32_t t : tris)
                indices.push_back(static_cast<GlyphIndex>(base + t));
        }
    }

    static void buildString(std::vector<GlyphVertex>& verts,
                            std::vector<GlyphIndex>&  indices,
                            FontParser&               font,
                            const std::string&        text,
                            const GlyphStyle          style     = GlyphStyle::Filled,
                            const float               lineWidth = 0.012f)
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

            if (style == GlyphStyle::Filled) buildFilled (verts, indices, g, penX, upem);
            else                             buildOutline(verts, indices, g, penX, upem, lineWidth);

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