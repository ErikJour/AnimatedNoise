#pragma once
#include "webgpu/webgpu.h"

#if defined(__APPLE__)

struct MetalSurface
{
    WGPUSurface surface = nullptr;
    void*       view    = nullptr;
};

MetalSurface createMetalSurface(WGPUInstance instance, double contentsScale);
#endif