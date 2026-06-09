//
// Created by Erik Jourgensen on 5/12/26.
//

#ifndef ANIMATEDNOISE_SCENE_H
#define ANIMATEDNOISE_SCENE_H
#include <filesystem>
#include <webgpu/webgpu.h>
#include "sharedHelper.h"
#include "utilityHelper.h"
#include "ResourceManager.h"
#include "plane.h"
#include "particleSystem.h"
#include "MyUniforms.h"
#include "proceduralSlider.h"
#include "circularFloor.h"
#include "cameraState.h"
#include "dragState.h"
#include "yurtBeams.h"
#include "skylight.h"


struct vec3 { float x,y,z; };


static constexpr uint32_t MAX_PARTICLES = 2000;
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
        void terminate();
        void reloadShader();
        void setUniforms(WGPUQueue queue, WGPUBuffer uniformBuffer, float time);
        void renderFrame(float currentTime);
        void ConfigureVertexLayout();
        bool createParticlePipeline();
        void initializeScene();
        bool createPipeline();
        void initializeFloor();
        void initializeSkylight();
        void initializeBeams();

        void InitializeSlider(uint32_t& indexCount, WGPUBuffer& vertexBuffer, WGPUBuffer& indexBuffer, float wallRadius, float angle) const;
        void initializeParticles();
        void setSliderValue(int index, float value);
        void initializePlane();
        float getSliderValue(const int index) const { return mSliderValues[index]; }
        float sliderTopFraction()       const { return (1.0f - (kSpineMaxY + mSliderPos[1])) * 0.5f; }
        float sliderBottomFraction()    const { return (1.0f - (kSpineMinY + mSliderPos[1])) * 0.5f; }
        float sliderXFraction()         const { return (mSliderPos[0] + 1.0f) * 0.5f; }
        static float indicatorHalfFraction()   { return kIndicatorHalfY * 0.5f; }
        void setSurfaceFormat(WGPUTextureFormat format) { mSurfaceFormat = format; }
        WGPUTextureView getDepthTextureView() const { return mDepthTextureView; }
        WGPUColorTargetState getColorTarget() const { return mColorTarget; }
        WGPUFragmentState getFragmentState() const { return mFragmentState; }
        WGPUBlendState getBlendState() const { return mBlendState; }
        void updateDepthTexture(uint32_t width, uint32_t height);
        void updateViewMatrix();
        void onMouseButton(int button, bool isPressed, float xpos, float ypos);
        void onMouseMove(float xpos, float ypos);
        void onScroll(float deltaX, float deltaY);
        CameraState getCameraState() const { return mCameraState; }
        void setCameraState(const CameraState& s) { mCameraState = s; updateViewMatrix(); }

        void projectSliderBounds(const float screenW, const float screenH,
                         float& outCenterX, float& outTopY, float& outBottomY, const float angle) const
        {
            // Constants must match buildSliderGeometry
            const float wx = 0.9f * std::cos(angle);   // wallRadius * cos(centerAngle=0)
            const float wz = 0.9f * std::sin(angle);    // wallRadius * sin(centerAngle=0)
            constexpr float yTop    =  0.25f;
            constexpr float yBottom = -0.15f;
            constexpr float yMid    = (yTop + yBottom) * 0.5f;

            auto project = [&](const float x,
                                const float y,
                                const float z,
                                float& sx,
                                float& sy)
            {
                const float* m = mUniforms.viewProjMatrix;
                const float cx = m[0]*x + m[4]*y + m[8]*z  + m[12];
                const float cy = m[1]*x + m[5]*y + m[9]*z  + m[13];
                const float cw = m[3]*x + m[7]*y + m[11]*z + m[15];
                sx = ( cx/cw * 0.5f + 0.5f)        * screenW;
                sy = (1.0f - (cy/cw * 0.5f + 0.5f)) * screenH;
            };

            float dummy;
            project(wx, yMid,    wz, outCenterX, dummy);
            project(wx, yTop,    wz, dummy,      outTopY);
            project(wx, yBottom, wz, dummy,      outBottomY);
        }

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

        uint32_t                            mUniformStride       = 0;
        WGPUBindGroup                       mBindGroup           = nullptr;
        std::array<WGPUVertexAttribute, 3>  mVertexAttribs       = {};
        std::vector<WGPUVertexBufferLayout> mVertexBufferLayouts = {};

        WGPUShaderModule                    mShaderModule = {};
        WGPURenderPipelineDescriptor        mPipelineDesc = {};
        WGPUColorTargetState                mColorTarget = {};
        WGPUFragmentState                   mFragmentState = {};
        WGPUBlendState                      mBlendState = {};
        MyUniforms                          mUniforms = {};
        //Noise Slider
        WGPUBuffer                          mNoiseLevelSliderVertexBuffer = nullptr;
        WGPUBuffer                          mNoiseLevelSliderIndexBuffer  = nullptr;
        uint32_t                            mNoiseLevelSliderIndexCount   = 0;
        //Noise Density
        WGPUBuffer                          mNoiseDensitySliderVertexBuffer = nullptr;
        WGPUBuffer                          mNoiseDensitySliderIndexBuffer  = nullptr;
        uint32_t                            mNoiseDensitySliderIndexCount   = 0;
        //Comb Amount Slider
        WGPUBuffer                          mCombAmtSliderVertexBuffer = nullptr;
        WGPUBuffer                          mCombAmtSliderIndexBuffer  = nullptr;
        uint32_t                            mCombAmtSliderIndexCount   = 0;
        //LPG Resonance Slider
        WGPUBuffer                          mLpgRezSliderVertexBuffer = nullptr;
        WGPUBuffer                          mLpgRezSliderIndexBuffer  = nullptr;
        uint32_t                            mLpgRezSliderIndexCount   = 0;

        float                               mSliderValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        float                               mSliderPos[3]    = { 0.5f, 0.0f, 0.2f };
        static constexpr float              kSpineMinY       = -0.15f;
        static constexpr float              kSpineMaxY       =  0.25f;
        static constexpr float              kIndicatorHalfY  =  0.025f;
        //Floor
        WGPUBuffer                          mFloorVertexBuffer  = nullptr;
        WGPUBuffer                          mFloorIndexBuffer  = nullptr;
        uint32_t                            mFloorIndexCount    = 0;
        //Skylight
        WGPUBuffer                          mSkylightVertexBuffer  = nullptr;
        WGPUBuffer                          mSkylightIndexBuffer  = nullptr;
        uint32_t                            mSkylightIndexCount    = 0;
        //Beams
        WGPUBuffer                          mBeamsVertexBuffer  = nullptr;
        WGPUBuffer                          mBeamsIndexBuffer  = nullptr;
        uint32_t                            mBeamsIndexCount    = 0;
        //Plane
        WGPUBuffer                          mPlaneVertexBuffer  = nullptr;
        WGPUBuffer                          mPlaneIndexBuffer  = nullptr;
        uint32_t                            mPlaneIndexCount    = 0;
        //Particle System
        WGPUBuffer                          mParticleQuadBuffer  = nullptr;
        WGPUBuffer                          mParticleDataBuffer  = nullptr;
        uint32_t                            mParticleCount    = 0;
        // Particle Pipeline
        WGPURenderPipeline                      mParticlePipeline     = nullptr;
        WGPURenderPipelineDescriptor            mParticlePipelineDesc {};
        std::array<WGPUVertexAttribute, 5>      mParticleVertexAttribs {};
        std::vector<WGPUVertexBufferLayout>     mParticleVertexBufferLayouts;
        WGPUFragmentState                       mParticleFragmentState     {};   // ← needed for fs_particle
        uint32_t                                mParticleDrawCount = 500;        // current visible count

        //Camera
        CameraState mCameraState;
        DragState mDrag;

        mutable double mRed = {};
        double mGreen = {};
        double mBlue = {};

};


#endif //ANIMATEDNOISE_SCENE_H