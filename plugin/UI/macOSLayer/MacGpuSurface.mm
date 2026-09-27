#import <QuartzCore/CAMetalLayer.h>
#import <Cocoa/Cocoa.h>
#include "webgpu/webgpu.h"
#include "MacGpuSurface.h"

//==========================================================================
//NSView with MetalLayer. Interactions fall through to the JUCE window layer
//==========================================================================
@interface AnimatedMetalView : NSView
@end

@implementation AnimatedMetalView
//=================
//No hit testing
//=================
- (NSView*)hitTest:(NSPoint)point   { (void) point; return nil; }
- (BOOL)acceptsFirstResponder       { return NO; }
@end

MetalSurface createMetalSurface(WGPUInstance instance, double contentsScale) {
    MetalSurface result      = {};

    AnimatedMetalView* view  = [[AnimatedMetalView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.contentsScale = contentsScale > 0.0 ? contentsScale : 1.0;
    metalLayer.opaque        = YES;

    view.layer               = metalLayer;
    view.wantsLayer          = YES;

    WGPUSurfaceSourceMetalLayer metalSource = {};
    metalSource.chain.next                  = nullptr;
    metalSource.chain.sType                 = WGPUSType_SurfaceSourceMetalLayer;
    metalSource.layer                       = (__bridge void*) metalLayer;

    WGPUSurfaceDescriptor surfaceDesc = {};
    surfaceDesc.nextInChain = &metalSource.chain;
    //========================================
    //Assign our surface -> drawable pixel buffer
    //=======================================
    result.surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
    //===========================================
    //Assign our NSView here -> our canvas
    //===========================================
    result.view    = (void*) [view autorelease];

    return result;
}
