//
// Created by Erik Jourgensen on 4/28/26.
//

#pragma once
#include <iostream>
#include <webgpu/webgpu.h>
#include <Shader.h>
#include "utilityHelper.h"
#include "GpuSurface.h"

#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

class WebGpuWindow
{
public:
    WebGpuWindow();
    ~WebGpuWindow();

    bool initialize();
    bool initSurface(void* nativeHandle, uint32_t width, uint32_t height);
    bool createPipeline();

    // Call from juce::Timer::timerCallback() at 60 Hz. Application::MainLoop() in tutorial
    void renderFrame() const;
    // Call from PluginEditor::resized().
    void onResize(uint32_t width, uint32_t height);
    [[nodiscard]] bool hasSurface() const { return mSurface != nullptr; }
    void terminate();

    //==============================================================================
    // Diagnostic helpers
    //==============================================================================
    static void setFeatures(WGPUAdapter adapter);
    void getAdapter(WGPUAdapter adapter, const WGPUAdapterInfo& properties);
    static void getLimits(WGPUAdapter adapter, WGPUSupportedLimits &limits);

private:
    // Queries surface caps on first call (using mAdapter), then releases mAdapter.
    // Subsequent calls (from onResize) skip the caps query and just reconfigure.
    void applySurfaceConfig(uint32_t width, uint32_t height)
    {
        if (mSurfaceFormat == WGPUTextureFormat_Undefined) {
            WGPUSurfaceCapabilities caps = {};
            wgpuSurfaceGetCapabilities(mSurface, mAdapter, &caps);
            mSurfaceFormat = caps.formats[0];
            wgpuSurfaceCapabilitiesFreeMembers(caps);
            wgpuAdapterRelease(mAdapter);
            mAdapter = nullptr;
        }

        WGPUSurfaceConfiguration config = {};
        config.device       = mDevice;
        config.format       = mSurfaceFormat;
        config.usage        = WGPUTextureUsage_RenderAttachment;
        config.width        = width;
        config.height       = height;
        config.presentMode  = WGPUPresentMode_Fifo;
        config.alphaMode    = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(mSurface, &config);
    }

    WGPUInstanceDescriptor       descriptor    = {};
    WGPUInstance                 mInstance     = nullptr;
    WGPURequestAdapterOptions    adapterOpts   = {};
    WGPUAdapterInfo              initProperties = {};
    WGPUAdapter                  mAdapter      = nullptr;
    WGPUDevice                   mDevice       = nullptr;
    WGPUQueue                    mQueue        = nullptr;
    WGPUSurface                  mSurface      = nullptr;
    WGPUTextureFormat            mSurfaceFormat = WGPUTextureFormat_Undefined;
    WGPUSupportedLimits          supportedLimits = {};
    WGPURenderPipelineDescriptor pipelineDesc = {};
    WGPURenderPipeline           mPipeline = {};
    WGPUFragmentState            fragmentState = {};
    WGPUBlendState               blendState = {};
    WGPUColorTargetState         colorTarget = {};
    WGPUShaderModuleDescriptor   shaderDesc = {};
    WGPUShaderModule             mShaderModule = {};
    WGPUShaderModuleWGSLDescriptor mShaderCodeDesc{};


};

