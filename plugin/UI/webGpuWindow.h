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
    //==============================================================================
    // Diagnostic helpers
    //==============================================================================
    static void setFeatures(WGPUAdapter adapter);
    void getAdapter(WGPUAdapter adapter, const WGPUAdapterInfo& properties);
    static void getLimits(WGPUAdapter adapter, WGPUSupportedLimits &limits);
    void setUniforms(WGPUQueue queue, WGPUBuffer uniformBuffer, float time) const;
    void wgpuPollEvents([[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser);
    void InitializeBuffers();
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

        wgpuSurfaceConfigure(mSurface, &config);
    }
    static void setDefault(WGPULimits &limits);
    static WGPURequiredLimits GetRequiredLimits(WGPUAdapter adapter);
    void BufferTest();

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
    WGPUQueue                      mQueue        = nullptr;
    WGPUSurface                    mSurface      = nullptr;
    WGPUTextureFormat              mSurfaceFormat = WGPUTextureFormat_Undefined;
    WGPUSupportedLimits            mSupportedLimits = {};
    WGPURenderPipelineDescriptor   mPipelineDesc = {};
    WGPURenderPipeline             mPipeline = {};
    WGPUFragmentState              mFragmentState = {};
    WGPUBlendState                 mBlendState = {};
    WGPUColorTargetState           mColorTarget = {};
    WGPUShaderModuleDescriptor     mShaderDesc = {};
    WGPUShaderModule               mShaderModule = {};
    WGPUShaderModuleWGSLDescriptor mShaderCodeDesc{};
    WGPUBuffer                     mUniformBuffer = nullptr;
    WGPUBindGroup                  mBindGroup = nullptr;
    WGPUBuffer                     mBufferOne = nullptr;
    WGPUBuffer                     mBufferTwo = nullptr;
    std::vector<WGPUVertexBufferLayout> mVertexBufferLayouts = {};
    std::array<WGPUVertexAttribute, 2> mVertexAttribs = {};
    std::string                    mShaderSource;
    //Index Buffers
    WGPUBuffer mPointBuffer  = nullptr;
    WGPUBuffer mIndexBuffer  = nullptr;
    uint32_t   indexCount    = 0;
};

