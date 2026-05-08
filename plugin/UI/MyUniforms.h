#pragma once
#include <cstddef>
#include <cstdint>

struct MyUniforms {
    float time;
    float frequency;
    float amplitude;
    float sliderValue;   // was _pad0 — now drives fill + indicator
    float lightPos[3];
    float _pad1;         // still 32 bytes ✓
};

static_assert(sizeof(MyUniforms) % 16 == 0);

