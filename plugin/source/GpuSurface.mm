#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#include <webgpu/webgpu.h>
#include "GpuSurface.h"

WGPUSurface createMetalSurface(WGPUInstance instance, void* nsViewHandle) {
    NSView* nsView = (__bridge NSView*)nsViewHandle;
    [nsView setWantsLayer:YES];

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    [nsView setLayer:metalLayer];

    WGPUSurfaceSourceMetalLayer metalSource = {};
    metalSource.chain.next = nullptr;
    metalSource.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    metalSource.layer = (__bridge void*)metalLayer;

    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &metalSource.chain;

    return wgpuInstanceCreateSurface(instance, &surfaceDesc);
}
