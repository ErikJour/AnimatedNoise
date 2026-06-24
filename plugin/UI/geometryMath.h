//
// Shared geometry math primitives — no rendering or platform dependencies.
//
#pragma once
#include <cmath>
#include <cstdint>

namespace AnimatedLpg
{
    struct vec3 {
        float x, y, z;

        vec3 operator+(vec3 o) const { return {x+o.x, y+o.y, z+o.z}; }
        vec3 operator-(vec3 o) const { return {x-o.x, y-o.y, z-o.z}; }
        vec3 operator*(float s) const { return {x*s, y*s, z*s}; }

        float dot(vec3 o)   const { return x*o.x + y*o.y + z*o.z; }
        vec3  cross(vec3 o) const { return { y*o.z - z*o.y,
                                             z*o.x - x*o.z,
                                             x*o.y - y*o.x }; }
        vec3 normalized() const {
            const float len = std::sqrt(dot(*this)) + 1e-8f;
            return { x/len, y/len, z/len };
        }
    };

    struct Frame { vec3 pos, tangent, b1, b2; };
}



