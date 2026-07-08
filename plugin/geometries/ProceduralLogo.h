//
// Created by Erik Jourgensen on 5/28/26.
//

// logoGeometry.h
// Created by Claude, based on pixel analysis of ERIK-JOURGENSEN_ICON_B_W.png
//
// Procedurally generates the Animated Instruments logo as a triangle mesh.
// No texture. No file I/O. Pure geometry, same vertex layout as perlinPlane.h.
//
// All coordinates normalized to [-0.5, 0.5] with (0,0) at logo center.
// Derived from 4000x4000px source PNG.
//
// Vertex layout matches perlinPlane.h:
//   @location(0) position : vec3f   -- offset  0
//   @location(2) normal   : vec3f   -- offset 12
//   @location(1) color    : vec3f   -- offset 24
#pragma once
#include <vector>
#include <cstdint>
#include <cmath>

#ifndef ANIMATEDNOISE_PERLINPLANE_H
struct PlaneVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b;
};
using PlaneIndex = uint16_t;
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  Measured constants  (tweak these to taste)
// ─────────────────────────────────────────────────────────────────────────────
namespace LogoConst
{
    static constexpr float SW   = 0.0150f;   // stroke half-width  (full = 0.030)

    // Rounded rectangle (centerlines of each wall stroke)
    static constexpr float RL   = -0.0750f;  // rect left
    static constexpr float RR   =  0.1540f;  // rect right
    static constexpr float RT   = -0.1870f;  // rect top
    static constexpr float RB   =  0.2600f;  // rect bottom
    static constexpr float RC   =  0.0650f;  // corner radius  ← adjust visually

    // Crossbar
    static constexpr float CB_Y = -0.0430f;  // crossbar y (centerline)

    // Three dots
    static constexpr float DOT_Y =  0.1480f;
    static constexpr float DOT_R =  0.0190f;
    static constexpr float DOT_X[3] = { -0.0200f, 0.0390f, 0.0990f };

    // Bottom stem
    static constexpr float STEM_X  =  0.0370f;
    static constexpr float STEM_Y0 =  0.2600f;  // top of stem = rect bottom
    static constexpr float STEM_Y1 =  0.3400f;  // stem end

    // Right cable
    static constexpr float RC_EXIT_Y  =  0.0360f;  // y where cable exits rect right wall
    static constexpr float RC_VERT_X  =  0.2960f;  // x of vertical cable section
    static constexpr float RC_VERT_Y0 =  0.0170f;  // bottom of vertical section
    static constexpr float RC_VERT_Y1 = -0.3450f;  // top of vertical section
    static constexpr float RC_HOOK_R  =  0.0300f;  // top hook corner radius
}

// ─────────────────────────────────────────────────────────────────────────────
//  Internal geometry helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace LogoImpl
{
    using F = float;
    static constexpr F PI = 3.14159265358979f;

    inline void pushV(std::vector<PlaneVertex>& verts,
                      F x, F y, F cr, F cg, F cb)
    {
        PlaneVertex v{};
        v.x = x; v.y = y; v.z = 0.0f;
        v.nx = 0.0f; v.ny = 0.0f; v.nz = 1.0f;
        v.r = cr; v.g = cg; v.b = cb;
        verts.push_back(v);
    }

    inline void pushQuad(std::vector<PlaneVertex>& verts,
                         std::vector<PlaneIndex>&  indices,
                         F x0, F y0,   // top-left
                         F x1, F y1,   // top-right
                         F x2, F y2,   // bottom-left
                         F x3, F y3,   // bottom-right
                         F cr, F cg, F cb)
    {
        auto base = static_cast<PlaneIndex>(verts.size());
        pushV(verts, x0, y0, cr, cg, cb);
        pushV(verts, x1, y1, cr, cg, cb);
        pushV(verts, x2, y2, cr, cg, cb);
        pushV(verts, x3, y3, cr, cg, cb);
        // Two CCW triangles
        indices.push_back(base+0); indices.push_back(base+2); indices.push_back(base+1);
        indices.push_back(base+1); indices.push_back(base+2); indices.push_back(base+3);
    }

    // ── Stroked straight line from (x0,y0) to (x1,y1), half-width = hw ──
    inline void strokeLine(std::vector<PlaneVertex>& verts,
                           std::vector<PlaneIndex>&  indices,
                           F x0, F y0, F x1, F y1, F hw,
                           F cr, F cg, F cb)
    {
        F dx = x1 - x0, dy = y1 - y0;
        F len = std::sqrt(dx*dx + dy*dy);
        if (len < 1e-6f) return;
        F px = -dy / len * hw;  // perpendicular
        F py =  dx / len * hw;
        pushQuad(verts, indices,
                 x0+px, y0+py,   x0-px, y0-py,
                 x1+px, y1+py,   x1-px, y1-py,
                 cr, cg, cb);
    }

    // ── Stroked arc: center (cx,cy), radius r, angles a0→a1 (radians),
    //    N = number of segments, hw = stroke half-width ──
    inline void strokeArc(std::vector<PlaneVertex>& verts,
                          std::vector<PlaneIndex>&  indices,
                          F cx, F cy, F r, F a0, F a1, int N, F hw,
                          F cr, F cg, F cb)
    {
        F prevX = cx + r * std::cos(a0);
        F prevY = cy + r * std::sin(a0);
        for (int i = 1; i <= N; ++i)
        {
            F t = static_cast<F>(i) / static_cast<F>(N);
            F a = a0 + t * (a1 - a0);
            F curX = cx + r * std::cos(a);
            F curY = cy + r * std::sin(a);
            strokeLine(verts, indices, prevX, prevY, curX, curY, hw, cr, cg, cb);
            prevX = curX; prevY = curY;
        }
    }

    // ── Filled circle (fan triangles) ──
    inline void filledCircle(std::vector<PlaneVertex>& verts,
                             std::vector<PlaneIndex>&  indices,
                             F cx, F cy, F r, int N,
                             F cr, F cg, F cb)
    {
        auto centerIdx = static_cast<PlaneIndex>(verts.size());
        pushV(verts, cx, cy, cr, cg, cb);
        for (int i = 0; i <= N; ++i)
        {
            F a = 2.0f * PI * static_cast<F>(i) / static_cast<F>(N);
            pushV(verts, cx + r * std::cos(a), cy + r * std::sin(a), cr, cg, cb);
        }
        for (int i = 0; i < N; ++i)
        {
            auto pi = static_cast<PlaneIndex>(centerIdx + 1 + i);
            indices.push_back(centerIdx);
            indices.push_back(pi);
            indices.push_back(static_cast<PlaneIndex>(pi + 1));
        }
    }

    // ── Stroked cubic bezier P0-P1-P2-P3, N segments ──
    inline void strokeCubicBezier(std::vector<PlaneVertex>& verts,
                                  std::vector<PlaneIndex>&  indices,
                                  F p0x, F p0y, F p1x, F p1y,
                                  F p2x, F p2y, F p3x, F p3y,
                                  int N, F hw,
                                  F cr, F cg, F cb)
    {
        auto eval = [&](F t, F& ox, F& oy) {
            F u = 1.0f - t;
            ox = u*u*u*p0x + 3*u*u*t*p1x + 3*u*t*t*p2x + t*t*t*p3x;
            oy = u*u*u*p0y + 3*u*u*t*p1y + 3*u*t*t*p2y + t*t*t*p3y;
        };
        F prevX, prevY;
        eval(0.0f, prevX, prevY);
        for (int i = 1; i <= N; ++i)
        {
            F curX, curY;
            eval(static_cast<F>(i) / static_cast<F>(N), curX, curY);
            strokeLine(verts, indices, prevX, prevY, curX, curY, hw, cr, cg, cb);
            prevX = curX; prevY = curY;
        }
    }

    // ── Rounded rectangle outline ──
    // Center walls at (rl, rr, rt, rb), corner radius rc, stroke hw, N segs/corner
    inline void strokeRoundedRect(std::vector<PlaneVertex>& verts,
                                  std::vector<PlaneIndex>&  indices,
                                  F rl, F rr, F rt, F rb, F rc, int N, F hw,
                                  F cr, F cg, F cb)
    {
        // Straight sections (shortened by rc at each end for corners)
        strokeLine(verts, indices, rl+rc, rt,    rr-rc, rt,    hw, cr, cg, cb); // top
        strokeLine(verts, indices, rl+rc, rb,    rr-rc, rb,    hw, cr, cg, cb); // bottom
        strokeLine(verts, indices, rl,    rt+rc, rl,    rb-rc, hw, cr, cg, cb); // left
        strokeLine(verts, indices, rr,    rt+rc, rr,    rb-rc, hw, cr, cg, cb); // right

        // Corners: arc centers are inset by rc from each outer corner
        // Top-left:    center (rl+rc, rt+rc), arc from 180° to 270°
        strokeArc(verts, indices, rl+rc, rt+rc, rc,  PI,       3*PI/2, N, hw, cr, cg, cb);
        // Top-right:   center (rr-rc, rt+rc), arc from 270° to 360°
        strokeArc(verts, indices, rr-rc, rt+rc, rc,  3*PI/2,   2*PI,   N, hw, cr, cg, cb);
        // Bottom-right: center (rr-rc, rb-rc), arc from 0° to 90°
        strokeArc(verts, indices, rr-rc, rb-rc, rc,  0,        PI/2,   N, hw, cr, cg, cb);
        // Bottom-left: center (rl+rc, rb-rc), arc from 90° to 180°
        strokeArc(verts, indices, rl+rc, rb-rc, rc,  PI/2,     PI,     N, hw, cr, cg, cb);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public builder — mirrors the buildPlane() API
// ─────────────────────────────────────────────────────────────────────────────
class Logo
{
public:
    // r, g, b: stroke color  (default: the gray from the original PNG ≈ 0.6)
    // scale:   uniform scale applied to all coordinates
    static void buildLogo(std::vector<PlaneVertex>& verts,
                          std::vector<PlaneIndex>&  indices,
                          float scale = 1.0f,
                          float cr    = 0.6f,
                          float cg    = 0.6f,
                          float cb    = 0.6f)
    {
        verts.clear();
        indices.clear();

        using namespace LogoConst;
        using namespace LogoImpl;

        // Convenience: scale helper so all constants pass through uniformly
        auto s = [&](float v) { return v * scale; };

        const float hw  = s(SW);
        const int   ARC = 12;  // segments per corner arc

        // ── 1. Rounded rectangle outline ──────────────────────────────────
        strokeRoundedRect(verts, indices,
                          s(RL), s(RR), s(RT), s(RB), s(RC), ARC, hw,
                          cr, cg, cb);

        // ── 2. Crossbar ───────────────────────────────────────────────────
        strokeLine(verts, indices,
                   s(RL), s(CB_Y), s(RR), s(CB_Y), hw,
                   cr, cg, cb);

        // ── 3. Three dots ─────────────────────────────────────────────────
        for (int i = 0; i < 3; ++i)
            filledCircle(verts, indices,
                         s(DOT_X[i]), s(DOT_Y), s(DOT_R), 16,
                         cr, cg, cb);

        // ── 4. Bottom stem ────────────────────────────────────────────────
        strokeLine(verts, indices,
                   s(STEM_X), s(STEM_Y0), s(STEM_X), s(STEM_Y1), hw,
                   cr, cg, cb);

        // ── 5. Right cable ────────────────────────────────────────────────
        // 5a. Short horizontal exit from rect right wall
        strokeLine(verts, indices,
                   s(RR), s(RC_EXIT_Y), s(0.215f), s(RC_EXIT_Y), hw,
                   cr, cg, cb);

        // 5b. Curve: horizontal-to-vertical transition (cubic bezier)
        //     P0 exits rightward, P3 enters upward into vertical section
        strokeCubicBezier(verts, indices,
                          s(0.215f), s(RC_EXIT_Y),   // P0
                          s(0.280f), s(RC_EXIT_Y),   // P1  (horizontal tangent)
                          s(RC_VERT_X), s(0.020f),   // P2  (vertical tangent)
                          s(RC_VERT_X), s(RC_VERT_Y0), // P3
                          16, hw, cr, cg, cb);

        // 5c. Vertical section going up
        strokeLine(verts, indices,
                   s(RC_VERT_X), s(RC_VERT_Y0),
                   s(RC_VERT_X), s(RC_VERT_Y1 + RC_HOOK_R),
                   hw, cr, cg, cb);

        // 5d. Hook corner at the top (quarter arc turning left)
        strokeArc(verts, indices,
                  s(RC_VERT_X - RC_HOOK_R), s(RC_VERT_Y1 + RC_HOOK_R), // arc center
                  s(RC_HOOK_R),
                  0.0f, PI / 2.0f,   // 0° → 90° (right → up, then CCW to left)
                  // Note: adjust angle range if top hook curves differently
                  ARC, hw, cr, cg, cb);

        // ── 6. Left cable spiral ──────────────────────────────────────────
        // The cable exits the crossbar at (RL, CB_Y) going left,
        // makes a clockwise spiral loop, and terminates.
        // Modeled as two chained cubic beziers approximating the S-curve.
        // Tune these control points to match your logo exactly.

        // Segment A: exit from rect wall → outer arc of loop (going left-down)
        strokeCubicBezier(verts, indices,
                          s(RL),      s(CB_Y),      // P0: exits rect left wall
                          s(-0.220f), s(CB_Y),      // P1: continue leftward
                          s(-0.295f), s(-0.010f),   // P2: start curving down
                          s(-0.295f), s(0.034f),    // P3: leftmost point
                          20, hw, cr, cg, cb);

        // Segment B: loop turns — comes back right along the lower arc
        strokeCubicBezier(verts, indices,
                          s(-0.295f), s(0.034f),    // P0: leftmost point
                          s(-0.295f), s(0.095f),    // P1: continues curving
                          s(-0.200f), s(0.115f),    // P2: lower strand heading right
                          s(-0.120f), s(0.060f),    // P3: curls back upward
                          20, hw, cr, cg, cb);

        // Segment C: inner curl terminates (loose end of cable)
        strokeCubicBezier(verts, indices,
                          s(-0.120f), s(0.060f),    // P0
                          s(-0.080f), s(0.028f),    // P1: curling up
                          s(-0.085f), s(-0.010f),   // P2: approaching crossbar height
                          s(-0.100f), s(-0.020f),   // P3: loose end (inside loop)
                          12, hw, cr, cg, cb);
    }
};

