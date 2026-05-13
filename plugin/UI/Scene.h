//
// Created by Erik Jourgensen on 5/12/26.
//

#ifndef ANIMATEDNOISE_SCENE_H
#define ANIMATEDNOISE_SCENE_H
#include <filesystem>
#include <webgpu/webgpu.h>
#include "perlinCave.h"
#include "utilityHelper.h"
#include "ResourceManager.h"
#include "plane.h"
#include "particleSystem.h"
#include "MyUniforms.h"
#include "proceduralSlider.h"
#include "circularFloor.h"



static constexpr uint32_t MAX_PARTICLES = 1000;
#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

class Scene
{
    public:
        Scene();
        ~Scene();
        void init(WGPUDevice device, WGPUQueue queue);
        void setSurface(WGPUSurface surface);
        void setSurfaceSize(uint32_t width, uint32_t height) { mWidth = width; mHeight = height; }
        void setShaderModule(WGPUShaderModule shaderModule);
        void setPipelineDesc(WGPURenderPipelineDescriptor pipelineDesc);
        void setWindowColor();
        bool createShader();
        void InitializeProceduralCave();
        void terminate();
        void reloadShader();
        void setUniforms(WGPUQueue queue, const WGPUBuffer uniformBuffer, const float time) const;
        void renderFrame(const float currentTime);
        void ConfigureVertexLayout();
        bool createParticlePipeline();
        void initializeScene();
        bool createPipeline();
        void initializeFloor();
        void initializeSkylight();
        void InitializeSlider();
        void initializeParticles();
        void setSliderPosition(const float x, const float y, const float z);
        void setSliderValue(float v);
        void initializePlane();
        float getSliderValue() const { return mSliderValue; }
        float sliderTopFraction()       const { return (1.0f - (kSpineMaxY + mSliderPos[1])) * 0.5f; }
        float sliderBottomFraction()    const { return (1.0f - (kSpineMinY + mSliderPos[1])) * 0.5f; }
        float sliderXFraction()         const { return (mSliderPos[0] + 1.0f) * 0.5f; }
        static float indicatorHalfFraction()   { return kIndicatorHalfY * 0.5f; }
        void setSurfaceFormat(WGPUTextureFormat format) { mSurfaceFormat = format; }
        WGPUTextureView getDepthTextureView() const { return mDepthTextureView; }
        WGPUColorTargetState getColorTarget() const { return mColorTarget; }
        WGPUFragmentState getFragmentState() const { return mFragmentState; }
        WGPUBlendState getBlendState() const { return mBlendState; }

    private:
        void setItemBuffers(WGPUBuffer vertexBuffer, WGPUBuffer indexBuffer, uint32_t indexCount, uint32_t material, WGPURenderPassEncoder renderPass) const
        {
            if (vertexBuffer && indexBuffer && indexCount > 0)
            {
                const uint32_t offset = material * mUniformStride;
                wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 1, &offset);
                wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, wgpuBufferGetSize(vertexBuffer));
                wgpuRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(indexBuffer));
                wgpuRenderPassEncoderDrawIndexed(renderPass, indexCount, 1, 0, 0, 0);
            }
        }
        uint32_t mWidth{};
        uint32_t mHeight{};
        WGPUDevice                          mDevice       = nullptr;
        WGPUQueue                           mQueue        = nullptr;
        WGPURenderPipeline                  mPipeline     = {};
        WGPUSurface                         mSurface      = nullptr;
        WGPUTextureFormat                   mSurfaceFormat = WGPUTextureFormat_Undefined;
        std::vector<std::filesystem::path>  mShaderPaths;
        std::filesystem::file_time_type     mLastShaderWriteTime;
        WGPUBuffer                          mUniformBuffer = nullptr;
        WGPUTexture                         mDepthTexture     = nullptr;
        WGPUTextureView                     mDepthTextureView = nullptr; //Revisit this

        uint32_t                            mUniformStride    = 0;
        WGPUBindGroup                       mBindGroup = nullptr;
        std::array<WGPUVertexAttribute, 3>  mVertexAttribs = {};
        std::vector<WGPUVertexBufferLayout> mVertexBufferLayouts = {};

        WGPUShaderModule                    mShaderModule = {};
        WGPURenderPipelineDescriptor        mPipelineDesc = {};
        WGPUColorTargetState                mColorTarget = {};
        WGPUFragmentState                   mFragmentState = {};
        WGPUBlendState                      mBlendState = {};
        //Cave
        WGPUBuffer                          mCaveVertexBuffer  = nullptr;
        WGPUBuffer                          mCaveIndexBuffer  = nullptr;
        uint32_t                            mCaveIndexCount    = 0;
        //Slider
        WGPUBuffer                          mSliderVertexBuffer = nullptr;
        WGPUBuffer                          mSliderIndexBuffer  = nullptr;
        uint32_t                            mSliderIndexCount   = 0;
        float                               mSliderValue     = 0.5f;
        float                               mSliderPos[3]   = { 0.5f, 0.0f, 0.2f };
        static constexpr float kSpineMinY      = -0.15f;
        static constexpr float kSpineMaxY      =  0.25f;
        static constexpr float kIndicatorHalfY =  0.025f;
        //Floor
        WGPUBuffer  mFloorVertexBuffer  = nullptr;
        WGPUBuffer  mFloorIndexBuffer  = nullptr;
        uint32_t    mFloorIndexCount    = 0;
        //Skylight
        WGPUBuffer  mSkylightVertexBuffer  = nullptr;
        WGPUBuffer  mSkylightIndexBuffer  = nullptr;
        uint32_t    mSkylightIndexCount    = 0;
        //Plane
        WGPUBuffer  mPlaneVertexBuffer  = nullptr;
        WGPUBuffer  mPlaneIndexBuffer  = nullptr;
        uint32_t    mPlaneIndexCount    = 0;
        //Particle System
        WGPUBuffer  mParticleQuadBuffer  = nullptr;
        WGPUBuffer  mParticleDataBuffer  = nullptr;
        uint32_t    mParticleCount    = 0;
        // Particle Pipeline
        WGPURenderPipeline           mParticlePipeline     = nullptr;
        WGPURenderPipelineDescriptor mParticlePipelineDesc {};
        std::array<WGPUVertexAttribute, 5>      mParticleVertexAttribs {};
        std::vector<WGPUVertexBufferLayout>     mParticleVertexBufferLayouts;
        WGPUFragmentState             mParticleFragmentState     {};   // ← needed for fs_particle
        uint32_t                        mParticleDrawCount = 500;        // current visible count


        mutable double mRed = {};
        double mGreen = {};
        double mBlue = {};

};


#endif //ANIMATEDNOISE_SCENE_H