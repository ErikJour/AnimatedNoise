#pragma once
#include <webgpu/webgpu.h>

#if defined(__APPLE__)
// A WGPU surface together with the dedicated NSView that backs it.
//
// We deliberately do NOT attach the CAMetalLayer to the window handle returned
// by Component::getWindowHandle(). In a DAW the editor owns its own peer, so
// that handle maps 1:1 to the editor. In Standalone the editor is a child of
// JUCE's StandaloneFilterWindow, so getWindowHandle() returns the whole window's
// content view (title bar included) and the Metal layer would paint over it.
//
// Instead we create our own NSView, hand it back here, and let the C++ side wrap
// it in a juce::NSViewComponent so it is sized/clipped to the editor in both
// hosts. The view is transparent to mouse events so JUCE keeps routing input.
struct MetalSurface
{
    WGPUSurface surface = nullptr; // owned by the caller (released via WGPU)
    void*       view    = nullptr; // NSView*, autoreleased; hand to NSViewComponent::setView
};

// contentsScale should be the backing scale factor (e.g. 2.0 on Retina).
MetalSurface createMetalSurface(WGPUInstance instance, double contentsScale);
#endif