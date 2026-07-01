// particleSystem.h
// Created by Erik Jourgensen on 5/12/26.

#ifndef ANIMATEDNOISE_PARTICLESYSTEM_H
#define ANIMATEDNOISE_PARTICLESYSTEM_H

#include <vector>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <random>

struct QuadVertex {
    float cx, cy;
    float u, v;
};

struct ParticleData {
    float x, y, z;
    float size;

    float r, g, b, a;

    float life;
    float vx, vy, vz;
};

class ParticleSystem
{
public:
    float height = -0.25f;

    static void buildQuad(std::vector<QuadVertex>& verts)
    {
        verts.clear();
        verts.reserve(6);

        verts.push_back({ -0.5f, -0.5f,  0.0f, 0.0f });
        verts.push_back({  0.5f, -0.5f,  1.0f, 0.0f });
        verts.push_back({  0.5f,  0.5f,  1.0f, 1.0f });

        verts.push_back({ -0.5f, -0.5f,  0.0f, 0.0f });
        verts.push_back({  0.5f,  0.5f,  1.0f, 1.0f });
        verts.push_back({ -0.5f,  0.5f,  0.0f, 1.0f });
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

            if (x*x + y*y + z*z > spread * spread) continue;

            ParticleData p{};
            p.x = x;
            p.y = y;
            p.z = z;
            p.size = size * (0.5f + 1.5f * randomFloat());
            p.r = 1.0f; p.g = 1.0f; p.b = 1.0f; p.a = 1.0f;
            p.life = randomFloat();
            p.vx = 0.0f; p.vy = 0.0f; p.vz = 0.0f;

            particles.push_back(p);
            ++spawned;
        }

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