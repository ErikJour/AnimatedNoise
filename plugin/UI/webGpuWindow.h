//
// Created by Erik Jourgensen on 4/28/26.
//

#pragma once
#include <iostream>
#include <webgpu/webgpu.h>
#include <Shader.h>
#include "utilityHelper.h"
#include "GpuSurface.h"

struct MyUniforms {
    float time = 0.0f;
    float frequency = 10.0f;
    float amplitude = 0.5f;
    float _pad = 0.0f;
};

#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

class WebGpuWindow
{
public:
    WebGpuWindow();
    ~WebGpuWindow();

    void setWindowColor();
    bool initialize();
    bool initSurface(void* nativeHandle, uint32_t width, uint32_t height);
    bool createPipeline();

    // Call from juce::Timer::timerCallback() at 60 Hz. Application::MainLoop() in tutorial
    void renderFrame(float currentTime) const;
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
    void setUniforms(WGPUQueue queue, WGPUBuffer uniformBuffer, float time) const;


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

    //======================================================
    //Colors
    //======================================================
    mutable double mRed = {};
    double mGreen = {};
    double mBlue = {};

    WGPUInstanceDescriptor         mDescriptor    = {};
    WGPUInstance                   mInstance     = nullptr;
    WGPURequestAdapterOptions      mAdapterOpts   = {};
    WGPUAdapterInfo                mInitProperties = {};
    WGPUAdapter                    mAdapter      = nullptr;
    WGPUDevice                     mDevice       = nullptr;
    WGPUQueue                      mQueue        = nullptr;
    WGPUSurface                    mSurface      = nullptr;
    WGPUTextureFormat              mSurfaceFormat = WGPUTextureFormat_Undefined;
    WGPUSupportedLimits            mSupportedLimits = {};
    WGPURenderPipelineDescriptor   mPipelineDesc = {};
    WGPURenderPipeline             mPipeline = {};
    WGPUFragmentState              mFragmentState = {};
    WGPUBlendState                 mBlendState = {};
    WGPUColorTargetState           mTargetColor = {};
    WGPUShaderModuleDescriptor     mShaderDesc = {};
    WGPUShaderModule               mShaderModule = {};
    WGPUShaderModuleWGSLDescriptor mShaderCodeDesc{};
    WGPUBuffer                     mUniformBuffer = nullptr;
    WGPUBindGroup                  mBindGroup = nullptr;


};

