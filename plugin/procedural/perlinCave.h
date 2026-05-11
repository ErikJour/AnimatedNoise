//
// Created by Erik Jourgensen on 5/11/26.
//

#ifndef ANIMATEDNOISE_PERLINCAVE_H
#define ANIMATEDNOISE_PERLINCAVE_H
#include <vector>   // Fixes 'std::vector' undeclared
#include <cmath>    // Fixes math functions
#include <cstdint>  // Fixes 'uint16_t' unknown type

// --- 3D Geometry Structures ---
struct Vertex { float x, y, z, nx, ny, nz, r, g, b; };
using Index = uint16_t;

struct Vec3 { float r, g, b; };
struct RingDef {
    float depth, scaleX, scaleY;
    Vec3 colorBottom, colorTop;
};

typedef struct { float x, y; } vector2;

class perlinCave
{
    public:
    perlinCave();
    ~perlinCave();
    static float perlin(float x, float y);
    static float dotGridGradient(int ix, int iy, float x, float y);
    static vector2 randomGradient(int ix, int iy);
    static float interpolate (const float a0, const float a1, const float w);
    // 3D Geometry Methods
    static void buildCaveGeometry(std::vector<Vertex>& verts, std::vector<Index>& indices);
    static Vec3 lerpColor(Vec3 a, Vec3 b, float t);

};


#endif //ANIMATEDNOISE_PERLINCAVE_H