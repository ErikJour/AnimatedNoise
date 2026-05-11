//
// Created by Erik Jourgensen on 4/28/26.
//

#pragma once
#include <iostream>
#include <webgpu/webgpu.h>
#include <Shader.h>
#include "utilityHelper.h"
#include "GpuSurface.h"
#include "shaderLoader.h"
#include "shader.wgsl.h"
#include <filesystem>
#include "MyUniforms.h"
#include "ResourceManager.h"
#include "perlinCave.h"

#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

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
    bool createShader();
    void configurePipeline();

    bool initialize();
    bool initSurface(void* nativeHandle, uint32_t width, uint32_t height);
    bool createPipeline();
    void renderFrame(float currentTime);
    void onResize(uint32_t width, uint32_t height);
    [[nodiscard]] bool hasSurface() const { return mSurface != nullptr; }
    void terminate();
    static void setFeatures(WGPUAdapter adapter);
    void getAdapter(WGPUAdapter adapter, const WGPUAdapterInfo& properties);
    static void getLimits(WGPUAdapter adapter, WGPUSupportedLimits &limits);
    void setUniforms(WGPUQueue queue, WGPUBuffer uniformBuffer, float time) const;
    void wgpuPollEvents([[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser);
    void ConfigureVertexLayout();
    void setSliderValue(float v) { mSliderValue = v; }
    float getSliderValue() const { return mSliderValue; }
    void setSliderPosition(float x, float y, float z);
    // void InitializeSlider();
    void InitializeProceduralCave();
    void InitializeLoadedCave();

    float sliderTopFraction()       const { return (1.0f - (kSpineMaxY + mSliderPos[1])) * 0.5f; }
    float sliderBottomFraction()    const { return (1.0f - (kSpineMinY + mSliderPos[1])) * 0.5f; }
    float sliderXFraction()         const { return (mSliderPos[0] + 1.0f) * 0.5f; }
    float indicatorHalfFraction()   const { return kIndicatorHalfY * 0.5f; }


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
        config.presentMode  = WGPUPresentMode_Fifo;
        config.alphaMode    = WGPUCompositeAlphaMode_Auto;

        if (mDepthTextureView) { wgpuTextureViewRelease(mDepthTextureView); mDepthTextureView = nullptr; }
        if (mDepthTexture)     { wgpuTextureDestroy(mDepthTexture); wgpuTextureRelease(mDepthTexture); mDepthTexture = nullptr; }

        WGPUTextureFormat depthFormat = WGPUTextureFormat_Depth24Plus;

        WGPUTextureDescriptor depthTexDesc   = {};
        depthTexDesc.dimension               = WGPUTextureDimension_2D;
        depthTexDesc.format                  = depthFormat;
        depthTexDesc.mipLevelCount           = 1;
        depthTexDesc.sampleCount             = 1;
        depthTexDesc.size = { width, height, 1 };

        depthTexDesc.usage                   = WGPUTextureUsage_RenderAttachment;
        depthTexDesc.viewFormatCount         = 1;
        depthTexDesc.viewFormats             = &depthFormat;
        mDepthTexture = wgpuDeviceCreateTexture(mDevice, &depthTexDesc);

        WGPUTextureViewDescriptor depthViewDesc = {};
        depthViewDesc.format                    = depthFormat;
        depthViewDesc.dimension                 = WGPUTextureViewDimension_2D;
        depthViewDesc.aspect                    = WGPUTextureAspect_DepthOnly;
        depthViewDesc.mipLevelCount             = 1;
        depthViewDesc.arrayLayerCount           = 1;
        mDepthTextureView = wgpuTextureCreateView(mDepthTexture, &depthViewDesc);

        wgpuSurfaceConfigure(mSurface, &config);
    }
    static void setDefault(WGPULimits &limits);
    static void setDefault(WGPUStencilFaceState& stencilFaceState);   // ADD
    static void setDefault(WGPUDepthStencilState& depthStencilState); // ADD
    static WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter);
#ifdef DEBUG
    void reloadShader();
#endif

    std::filesystem::path mShaderPath;
    std::filesystem::file_time_type mLastShaderWriteTime;
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
    WGPURenderPipeline             mPipeline = {};
    WGPUFragmentState              mFragmentState = {};
    WGPUBlendState                 mBlendState = {};
    WGPUColorTargetState           mColorTarget = {};
    WGPUShaderModule               mShaderModule = {};
    WGPUBuffer                     mUniformBuffer = nullptr;
    WGPUBindGroup                  mBindGroup = nullptr;
    WGPUDepthStencilState          depthStencilState = {};
    WGPUTexture                    mDepthTexture     = nullptr;
    WGPUTextureView                mDepthTextureView = nullptr;
    perlinCave                     mPerlinCave = {};

    std::vector<WGPUVertexBufferLayout> mVertexBufferLayouts = {};
    std::array<WGPUVertexAttribute, 3> mVertexAttribs = {};
    //Cave
    WGPUBuffer  mCaveVertexBuffer  = nullptr;
    WGPUBuffer  mCaveIndexBuffer  = nullptr;
    uint32_t    mCaveIndexCount    = 0;
    //Slider
    WGPUBuffer  mSliderVertexBuffer = nullptr;
    WGPUBuffer  mSliderIndexBuffer  = nullptr;
    uint32_t    mSliderIndexCount   = 0;
    float       mSliderValue     = 0.5f;
    float       mSliderPos[3]   = { 0.5f, 0.0f, 0.2f };

    static constexpr float kSpineMinY      = -0.15f;
    static constexpr float kSpineMaxY      =  0.25f;
    static constexpr float kIndicatorHalfY =  0.025f;


};

