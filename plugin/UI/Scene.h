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
#include "../shaders/MyUniforms.h"
#include "circularFloor.h"
#include "components/camera/cameraState.h"
#include "components/mouse/dragState.h"
#include "skylight.h"
#include "components/logo/AnimatedLogo.h"
#include "sphereGeometry.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "components/slider/AnimatedSlider.h"
#include "ParamIds.h"
#include "components/camera/cameraHelper.h"
#include "shaderPaths.h"
#include "glyphGeometry.h"
#include "components/text/FontParser.h"
#include <sphericalSlider.h>

static constexpr uint32_t MAX_PARTICLES = 2000;
constexpr auto fontPath = "/Users/erikjourgensen/Desktop/July 2026/Repositories/AnimatedNoise/plugin/UI/fonts/WorkSans-Regular.ttf";
#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

class Scene
{
    public:
        Scene();
        ~Scene();
        void init(WGPUDevice device, WGPUQueue queue);
        void setSurface(WGPUSurface surface);
        void setSurfaceSize(uint32_t width, uint32_t height);
        void setShaderModule(WGPUShaderModule shaderModule);
        void setPipelineDesc(const WGPURenderPipelineDescriptor& pipelineDesc);
        bool createShader();
        void terminate();
        void reloadShader();
        void setUniforms(WGPUQueue queue, WGPUBuffer uniformBuffer, float time);
        void setSliderUniforms(WGPUQueue queue, WGPUBuffer uniformBuffer);
        std::pair<WGPUSurfaceTexture, WGPUTextureView> getNextSurfaceViewData() const;
        void renderMeshes(WGPURenderPassEncoder renderPass);
        void renderFrame(float currentTime);
        void ConfigureVertexLayout();
        bool createParticlePipeline();
        void initializeScene();
        bool createPipeline();
        void initializeFloor();
        void initializeSphere();
        void InitializeSlider(uint32_t& indexCount, WGPUBuffer& vertexBuffer, WGPUBuffer& indexBuffer, float radius) const;
        void initializeSkylight();
        void initializeParticles();
        void initializeText(FontParser& font, std::string text);
        void initializeTooltip(FontParser& font, const std::string& paramName, const std::string& paramValue);
        void uploadGlyphMesh(const std::vector<GlyphVertex>& vertices, const std::vector<GlyphIndex>&  indices);
        void uploadTooltipMesh(const std::vector<GlyphVertex>& vertices, const std::vector<GlyphIndex>&  indices);
        void setSurfaceFormat(WGPUTextureFormat format);
        void setCameraState(const CameraState& s);
        void updateDepthTexture(uint32_t width, uint32_t height);
        void updateViewMatrix();
        void onMouseButton(int button, bool isPressed, float xpos, float ypos);
        void onMouseMove(float xpos, float ypos);
        void onScroll(float deltaX, float deltaY);
        void setToolTip(const std::string &paramName, const std::string &paramValue);
        void initializeLightHelper();
        void setSliderList(const std::vector<AnimatedSlider>& list) { mSliderList = &list; }
        const float* invView() const { return mInvView; }
        const float* invProj() const { return mInvProj; }
        static void buildInvLookAt(float* out,float ex, float ey, float ez, float tx, float ty, float tz,
                                                   float upx = 0.0f, float upy = 1.0f, float upz = 0.0f);
        static void buildInvPerspective(float* out, float fovY, float aspect, float nearZ, float farZ);
        static void makeModelMatrix(float* m, float angle, float tx, float ty, float tz);
        void setItemBuffers(WGPUBuffer vertexBuffer, WGPUBuffer indexBuffer, uint32_t indexCount,
                                            uint32_t material, WGPURenderPassEncoder renderPass) const;
        const AnimatedSlider* findSlider(const juce::ParameterID& id) const;
        //=================================================================================================
        WGPUTextureView getDepthTextureView() const { return mDepthTextureView; }
        WGPUColorTargetState getColorTarget() const { return mColorTarget; }
        WGPUFragmentState getFragmentState()  const { return mFragmentState; }
        WGPUBlendState getBlendState()        const { return mBlendState; }
        CameraState getCameraState()          const { return mCameraState; }

    private:
        //=========================================================
        //Variables
        //=========================================================
        uint32_t mWidth{};
        uint32_t mHeight{};
        WGPUDevice                          mDevice              = nullptr;
        WGPUQueue                           mQueue               = nullptr;
        WGPURenderPipeline                  mPipeline            = {};
        WGPUSurface                         mSurface             = nullptr;
        WGPUTextureFormat                   mSurfaceFormat       = WGPUTextureFormat_Undefined;
        std::vector<std::filesystem::path>  mShaderPaths;
        std::filesystem::file_time_type     mLastShaderWriteTime;
        WGPUBuffer                          mUniformBuffer       = nullptr;
        WGPUTexture                         mDepthTexture        = nullptr;
        WGPUTextureView                     mDepthTextureView    = nullptr; //Revisit this

        uint32_t                            mUniformStride       = 0;
        WGPUBindGroup                       mBindGroup           = nullptr;
        std::array<WGPUVertexAttribute, 3>  mVertexAttribs       = {};
        std::vector<WGPUVertexBufferLayout> mVertexBufferLayouts = {};

        WGPUShaderModule                    mShaderModule        = {};
        WGPURenderPipelineDescriptor        mPipelineDesc        = {};
        WGPUColorTargetState                mColorTarget         = {};
        WGPUFragmentState                   mFragmentState       = {};
        WGPUBlendState                      mBlendState          = {};
        MyUniforms                          mUniforms            = {};

        struct SliderMesh {
            WGPUBuffer vertexBuffer = nullptr;
            WGPUBuffer indexBuffer  = nullptr;
            uint32_t   indexCount   = 0;
            uint32_t   materialId   = 0;
        };

        std::vector<SliderMesh>             mSliderMeshes;
        static constexpr float              kSpineMinY                 = -0.15f;
        static constexpr float              kSpineMaxY                 =  0.25f;
        static constexpr float              kIndicatorHalfY            =  0.025f;
        //Plane
        WGPUBuffer                          mLightHelperVertexBuffer   = nullptr;
        WGPUBuffer                          mLightHelperIndexBuffer    = nullptr;
        uint32_t                            mLightHelperIndexCount     = 0;
        //Floor
        WGPUBuffer                          mFloorVertexBuffer         = nullptr;
        WGPUBuffer                          mFloorIndexBuffer          = nullptr;
        uint32_t                            mFloorIndexCount           = 0;
        //Sphere
        WGPUBuffer                          mSphereVertexBuffer        = nullptr;
        WGPUBuffer                          mSphereIndexBuffer         = nullptr;
        uint32_t                            mSphereIndexCount          = 0;
        //Skylight
        WGPUBuffer                          mSkylightVertexBuffer      = nullptr;
        WGPUBuffer                          mSkylightIndexBuffer       = nullptr;
        uint32_t                            mSkylightIndexCount        = 0;
        //Particle System
        WGPUBuffer                          mParticleQuadBuffer        = nullptr;
        WGPUBuffer                          mParticleDataBuffer        = nullptr;
        uint32_t                            mParticleCount             = 0;
        // Particle Pipeline
        WGPURenderPipeline                  mParticlePipeline          = nullptr;
        WGPURenderPipelineDescriptor        mParticlePipelineDesc {};
        std::array<WGPUVertexAttribute, 5>  mParticleVertexAttribs {};
        std::vector<WGPUVertexBufferLayout> mParticleVertexBufferLayouts;
        WGPUFragmentState                   mParticleFragmentState {};
        uint32_t                            mParticleDrawCount         = 500;
        WGPUBlendState                      mParticleBlendState{};
        WGPUColorTargetState                mParticleColorTarget{};
        WGPUDepthStencilState               mParticleDepthStencil{};
        //Preset Name
        WGPUBuffer                          mPresetVertexBuffer        = nullptr;
        WGPUBuffer                          mPresetIndexBuffer         = nullptr;
        uint32_t                            mPresetIndexCount          = 0;
        //Tooltip Text
        WGPUBuffer                          mTooltipVertexBuffer       = nullptr;
        WGPUBuffer                          mTooltipIndexBuffer        = nullptr;
        uint32_t                            mTooltipIndexCount         = 0;

        std::string mText;
        FontParser mFont;

        AnimatedLogo mLogo;

        //Camera
        CameraState mCameraState;
        DragState mDrag;
        float mView[16]{}, mProj[16]{};
        float mInvView[16]{}, mInvProj[16]{};

        const std::vector<AnimatedSlider>* mSliderList = nullptr;

};


#endif //ANIMATEDNOISE_SCENE_H