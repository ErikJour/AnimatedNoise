//
// Created by Erik Jourgensen on 4/28/26.
//

#pragma once
#include <webgpu/webgpu.h>
#include "shaderLoader.h"
#include "Scene.h"


class WebGpuWindow
{
public:
    WebGpuWindow();
    ~WebGpuWindow();

    void setWindowColor();
    bool createInstance();
    bool createAdapter();
    bool createDevice();
    bool createQueue();
    void configurePipeline();

    bool initialize();
    bool initSurface(void* nativeHandle, uint32_t width, uint32_t height);
    void onResize(uint32_t width, uint32_t height);
    [[nodiscard]] bool hasSurface() const { return mSurface != nullptr; }
    void terminate();
    static void setFeatures(WGPUAdapter adapter);
    void getAdapter(WGPUAdapter adapter, const WGPUAdapterInfo& properties);
    static void getLimits(WGPUAdapter adapter, WGPUSupportedLimits &limits);
    Scene& getScene() { return mScene; }


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
    static void setDefault(WGPUStencilFaceState& stencilFaceState);   // ADD
    static void setDefault(WGPUDepthStencilState& depthStencilState); // ADD
    static WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter);

    //====================================
    //Variables
    //====================================
    mutable double mRed = {};
    double mGreen = {};
    double mBlue = {};
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
    WGPUDepthStencilState          depthStencilState = {};


    Scene mScene;

};

