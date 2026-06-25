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
#include "AnimatedLogo.h"
#include "sphereGeometry.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "AnimatedSlider.h"


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
        void initializeSphere();
        void initializeSkylight();
        void InitializeSlider(uint32_t& indexCount, WGPUBuffer& vertexBuffer, WGPUBuffer& indexBuffer, float wallRadius, float angle) const;
        void initializeParticles();
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
                 juce::Point<float>& outTop,
                 juce::Point<float>& outBottom,
                 const float angle) const
        {
            outTop    = projectSliderPoint(screenW, screenH, 1.0f, angle);
            outBottom = projectSliderPoint(screenW, screenH, 0.0f, angle);
        }
    void setSliderList(const std::vector<AnimatedSlider>& list) { mSliderList = &list; }

    // Project the spine centerline at height-fraction v (0 = bottom, 1 = top)
    // to screen pixels. v maps to the SAME world-y span the geometry is built
    // over (buildSliderGeometry yBottom/yTop), so the bead/tube hit positions
    // track the rendered tube exactly — including perspective foreshortening,
    // which a straight screen-space lerp between the endpoints does not.
    juce::Point<float> projectSliderPoint(const float screenW, const float screenH,
                                          const float v, const float angle) const
        {
            const float wx = 0.9f * std::cos(angle);
            const float wz = 0.9f * std::sin(angle);
            constexpr float yTop    =  0.28125f;
            constexpr float yBottom = -0.01875f;
            const float y = yBottom + v * (yTop - yBottom);

            const float* m = mUniforms.viewProjMatrix;
            const float cx = m[0]*wx + m[4]*y + m[8]*wz  + m[12];
            const float cy = m[1]*wx + m[5]*y + m[9]*wz  + m[13];
            const float cw = m[3]*wx + m[7]*y + m[11]*wz + m[15];
            return { (cx/cw * 0.5f + 0.5f)        * screenW,
                     (1.0f - (cy/cw * 0.5f + 0.5f)) * screenH };
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
        //Sphere
        WGPUBuffer                          mSphereVertexBuffer   = nullptr;
        WGPUBuffer                          mSphereIndexBuffer    = nullptr;
        uint32_t                            mSphereIndexCount     = 0;
        //Skylight
        WGPUBuffer                          mSkylightVertexBuffer  = nullptr;
        WGPUBuffer                          mSkylightIndexBuffer  = nullptr;
        uint32_t                            mSkylightIndexCount    = 0;
        //Particle System
        WGPUBuffer                          mParticleQuadBuffer  = nullptr;
        WGPUBuffer                          mParticleDataBuffer  = nullptr;
        uint32_t                            mParticleCount    = 0;
        // Particle Pipeline
        WGPURenderPipeline                  mParticlePipeline     = nullptr;
        WGPURenderPipelineDescriptor        mParticlePipelineDesc {};
        std::array<WGPUVertexAttribute, 5>  mParticleVertexAttribs {};
        std::vector<WGPUVertexBufferLayout> mParticleVertexBufferLayouts;
        WGPUFragmentState                   mParticleFragmentState     {};
        uint32_t                            mParticleDrawCount = 500;
        WGPUBlendState                      mParticleBlendState{};
        WGPUColorTargetState                mParticleColorTarget{};
        WGPUDepthStencilState               mParticleDepthStencil{};

        AnimatedLogo mLogo;

        //Camera
        CameraState mCameraState;
        DragState mDrag;

        mutable double mRed = {};
        double mGreen = {};
        double mBlue = {};

        const std::vector<AnimatedSlider>* mSliderList = nullptr;  // borrowed from SliderManager


        // WGPURenderPipeline mParticleWorldPipeline = nullptr;


};


#endif //ANIMATEDNOISE_SCENE_H