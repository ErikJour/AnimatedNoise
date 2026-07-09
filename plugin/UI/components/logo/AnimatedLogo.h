//
// Created by Erik Jourgensen on 6/15/26.
//

#pragma once
#include <webgpu/webgpu.h>
#include "shaderPaths.h"


class AnimatedLogo
{
    public:
        void init(WGPUDevice device, WGPUQueue queue);
        bool initialize(WGPUTextureFormat surfaceFormat, const WGPUDepthStencilState& depthStencilTemplate);
        void terminate();
        void render(WGPURenderPassEncoder renderPass) const;

    private:
        WGPUDevice         mDevice       = nullptr;
        WGPUQueue          mQueue        = nullptr;
        WGPUShaderModule   mShaderModule = nullptr;
        WGPUTexture        mTexture      = nullptr;
        WGPUTextureView    mTextureView  = nullptr;
        WGPUSampler        mSampler      = nullptr;
        WGPUBuffer         mVertexBuffer = nullptr;
        WGPUBuffer         mIndexBuffer  = nullptr;
        uint32_t           mIndexCount   = 0;
        WGPURenderPipeline mPipeline     = nullptr;
        WGPUBindGroup      mBindGroup    = nullptr;
    };



