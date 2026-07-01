//
// Created by Erik Jourgensen on 4/28/26.
//

#pragma once
#include <webgpu/webgpu.h>
#include "../shaders/shaderLoader.h"
#include "Scene.h"


class WebGpuWindow
{
public:
    WebGpuWindow();
    ~WebGpuWindow();

    bool createInstance();
    bool createAdapter();
    bool createDevice();
    bool createQueue();
    void configurePipeline();

    bool initialize();
    bool initSurface(double contentsScale, uint32_t width, uint32_t height);
    void onResize(uint32_t width, uint32_t height);
    void terminate();
    static void setFeatures(WGPUAdapter adapter);
    void getAdapter(WGPUAdapter adapter, const WGPUAdapterInfo& properties);
    static void getLimits(WGPUAdapter adapter, WGPUSupportedLimits &limits);

    Scene& getScene() { return mScene; }
    [[nodiscard]] bool hasSurface() const { return mSurface != nullptr; }
    [[nodiscard]] void* getNativeView() const { return mNativeView; }


private:
    void applySurfaceConfig(const uint32_t width, const uint32_t height)
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
        config.presentMode  = WGPUPresentMode_Immediate;
        config.alphaMode    = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(mSurface, &config);
    }
    static void setDefault(WGPULimits &limits);
    static void setDefault(WGPUStencilFaceState& stencilFaceState);
    static void setDefault(WGPUDepthStencilState& depthStencilState);
    static WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter);

    //====================================
    //Variables
    //====================================
    void*                          mNativeView        = nullptr;
    WGPUInstance                   mInstance     = nullptr;
    WGPUAdapterInfo                mInitProperties = {};
    WGPUAdapter                    mAdapter      = nullptr;
    WGPUDevice                     mDevice       = nullptr;
    WGPUQueue                      mQueue        = nullptr; //This is like my process block
    WGPUSurface                    mSurface      = nullptr;
    WGPUTextureFormat              mSurfaceFormat = WGPUTextureFormat_Undefined;
    WGPUSupportedLimits            mSupportedLimits = {};
    WGPURenderPipelineDescriptor   mPipelineDesc = {};
    WGPUFragmentState              mFragmentState = {};
    WGPUBlendState                 mBlendState = {};
    WGPUColorTargetState           mColorTarget = {};
    WGPUDepthStencilState          mDepthStencilState = {};

    Scene mScene;

};

