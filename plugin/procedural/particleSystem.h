// particleSystem.h
// Created by Erik Jourgensen on 5/12/26.

#ifndef ANIMATEDNOISE_PARTICLESYSTEM_H
#define ANIMATEDNOISE_PARTICLESYSTEM_H

#include <vector>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <random>

// ─────────────────────────────────────────────────────────────────────────────
// BUFFER 1 — QuadVertex  (vertex buffer, step mode: per-vertex)
//
// A single unit quad shared by every particle instance.
// The vertex shader offsets each corner in view space to achieve billboarding.
//
// WGSL vertex layout:
//   @location(0) cornerOffset : vec2f   -- offset  0, stride 16
//   @location(1) uv           : vec2f   -- offset  8
//
// Topology: triangle-list, 6 vertices (two CCW triangles, no index buffer needed)
// ─────────────────────────────────────────────────────────────────────────────
struct QuadVertex {
    float cx, cy;   // corner offset in local space: ±0.5
    float u, v;     // UV coords: 0..1
};

// ─────────────────────────────────────────────────────────────────────────────
// BUFFER 2 — ParticleData  (storage buffer, step mode: per-instance)
//
// One entry per particle. Grouped into vec4f-sized chunks to satisfy
// WGSL 16-byte struct alignment without hidden padding surprises.
//
// WGSL storage struct:
//   struct Particle {
//       pos_size  : vec4f,   // xyz = world position, w = billboard size
//       color     : vec4f,   // rgba
//       life_vel  : vec4f,   // x = normalized lifetime 0..1,
//   }                        // yzw = velocity (for compute shader animation)
//
// Total: 48 bytes per particle
// ─────────────────────────────────────────────────────────────────────────────
struct ParticleData {
    // --- pos_size (vec4f) ---
    float x, y, z;      // world position
    float size;          // billboard scale in world units

    // --- color (vec4f) ---
    float r, g, b, a;   // RGBA color

    // --- life_vel (vec4f) ---
    float life;          // normalized lifetime: 1.0 = just born, 0.0 = dead
    float vx, vy, vz;   // velocity (consumed by compute shader, ignored in render-only mode)
};

// ─────────────────────────────────────────────────────────────────────────────
// ParticleSystem
//
// Responsibilities:
//   buildQuad()     — generate the shared unit billboard quad (called once)
//   initParticles() — scatter N particles into a ParticleData array (CPU init)
//
// GPU buffer creation and bind group wiring happen in the renderer, not here.
// ─────────────────────────────────────────────────────────────────────────────
class ParticleSystem
{
public:
    // Produces 6 QuadVertex entries describing a unit quad in CCW winding.
    // All particles share this single quad via instanced draw.
    //
    // Equivalent to: draw(6, particleCount) in the render pass.
    //
    //   3──2
    //   │ /│
    //   │/ │
    //   0──1
    //
    //   Triangle 0: 0, 1, 2  (bottom-left, bottom-right, top-right)
    //   Triangle 1: 0, 2, 3  (bottom-left, top-right,  top-left)
    static void buildQuad(std::vector<QuadVertex>& verts)
    {
        verts.clear();
        verts.reserve(6);

        // corner offsets and matching UVs
        // cx, cy,   u, v
        verts.push_back({ -0.5f, -0.5f,  0.0f, 0.0f }); // 0 bottom-left
        verts.push_back({  0.5f, -0.5f,  1.0f, 0.0f }); // 1 bottom-right
        verts.push_back({  0.5f,  0.5f,  1.0f, 1.0f }); // 2 top-right

        verts.push_back({ -0.5f, -0.5f,  0.0f, 0.0f }); // 0 bottom-left (repeated)
        verts.push_back({  0.5f,  0.5f,  1.0f, 1.0f }); // 2 top-right   (repeated)
        verts.push_back({ -0.5f,  0.5f,  0.0f, 1.0f }); // 3 top-left
    }

    static void initParticles(std::vector<ParticleData>& particles,
                          const int   count  = 500,
                          const float spread = 0.35f,
                          const float size   = 0.0075f)
    {
        particles.clear();
        particles.reserve(static_cast<size_t>(count));
        int spawned = 0;

        while (spawned < count)
        {
            float x = (randomFloat() - 0.5f) * spread * 3.14f;
            float y = (randomFloat() - 0.5f) * spread * 3.14f;
            float z = (randomFloat() - 0.5f) * spread * 3.14f;

            // Reject if outside sphere
            if (x*x + y*y + z*z > spread * spread) continue;

            ParticleData p{};
            p.x = x;
            p.y = y;
            p.z = z;
            p.size = size;
            p.r = 1.0f; p.g = 1.0f; p.b = 1.0f; p.a = 1.0f;
            p.life = randomFloat();
            p.vx = 0.0f; p.vy = 0.0f; p.vz = 0.0f;

            particles.push_back(p);
            ++spawned;
        }

        // Color variation pass
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-0.09f, 0.09f);

        for (auto& p : particles) {
            p.r = std::clamp(p.r + dist(rng), 0.0f, 1.0f);
            p.g = std::clamp(p.g + dist(rng), 0.0f, 1.0f);
            p.b = std::clamp(p.b + dist(rng), 0.0f, 1.0f);
        }
    }

private:
    static float randomFloat()
    {
        return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
    }
};

#endif // ANIMATEDNOISE_PARTICLESYSTEM_H