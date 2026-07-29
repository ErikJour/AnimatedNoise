#pragma once
#include <webgpu/webgpu.h>

#if defined(__APPLE__)

struct MetalSurface
{
    WGPUSurface surface = nullptr; // owned by the caller (released via WGPU)
    void*       view    = nullptr; // NSView*, autoreleased; hand to NSViewComponent::setView
};

MetalSurface createMetalSurface(WGPUInstance instance, double contentsScale);
#endif