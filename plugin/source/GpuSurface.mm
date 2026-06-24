#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#include <webgpu/webgpu.h>
#include "GpuSurface.h"

// An NSView that hosts the CAMetalLayer for the WebGPU scene. It is invisible to
// hit-testing, so every mouse event falls through to JUCE's peer view beneath it
// and the editor keeps receiving mouseDown/drag/etc. It also refuses
// first-responder status so it never steals keyboard focus.
@interface AnimatedMetalView : NSView
@end

@implementation AnimatedMetalView
- (NSView*)hitTest:(NSPoint)point   { (void) point; return nil; }
- (BOOL)acceptsFirstResponder       { return NO; }
@end

MetalSurface createMetalSurface(WGPUInstance instance, double contentsScale) {
    MetalSurface result = {};

    AnimatedMetalView* view = [[AnimatedMetalView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.contentsScale = contentsScale > 0.0 ? contentsScale : 1.0;
    metalLayer.opaque = YES;

    // Layer-HOSTING (assign the layer, then turn on wantsLayer). This mirrors the
    // original code path that attached the CAMetalLayer directly to JUCE's peer
    // view, so the scene renders in the exact same coordinate space/orientation.
    // A layer-BACKED view (makeBackingLayer) would let AppKit flip the layer's
    // geometry, mirroring the scene vertically and breaking slider hit-testing.
    view.layer = metalLayer;
    view.wantsLayer = YES;

    WGPUSurfaceSourceMetalLayer metalSource = {};
    metalSource.chain.next = nullptr;
    metalSource.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    metalSource.layer = (__bridge void*) metalLayer;

    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &metalSource.chain;

    result.surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);

    // MRR build (no -fobjc-arc): hand back a +0 autoreleased reference. The C++
    // caller passes it to NSViewComponent::setView synchronously on the message
    // thread, which retains it before this run loop's autorelease pool drains.
    result.view = (void*) [view autorelease];
    return result;
}
