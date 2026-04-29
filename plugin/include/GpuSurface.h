#pragma once
#include <webgpu/webgpu.h>

#if defined(__APPLE__)
WGPUSurface createMetalSurface(WGPUInstance instance, void* nsViewHandle);
#endif
