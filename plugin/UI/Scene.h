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
        void setPipelineDesc(WGPURenderPipelineDescriptor pipelineDesc);
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
        void InitializeSlider(uint32_t& indexCount, WGPUBuffer& vertexBuffer, WGPUBuffer& indexBuffer, float radius) const;
        void initializeSkylight();
        void initializeParticles();
        void initializeText(FontParser& font, const std::string& text);
        void uploadGlyphMesh(const std::vector<GlyphVertex>& vertices,
                             const std::vector<GlyphIndex>&  indices);


        void setSurfaceFormat(WGPUTextureFormat format);
        void setCameraState(const CameraState& s);
        float getSliderValue(int index) const;
        float sliderTopFraction() const;
        float sliderBottomFraction() const;
        float sliderXFraction() const;
        static float indicatorHalfFraction();
        void updateDepthTexture(uint32_t width, uint32_t height);
        void updateViewMatrix();
        void onMouseButton(int button, bool isPressed, float xpos, float ypos);
        void onMouseMove(float xpos, float ypos);
        void onScroll(float deltaX, float deltaY);



        void setSliderList(const std::vector<AnimatedSlider>& list) { mSliderList = &list; }

        WGPUTextureView getDepthTextureView() const { return mDepthTextureView; }
        WGPUColorTargetState getColorTarget() const { return mColorTarget; }
        WGPUFragmentState getFragmentState()  const { return mFragmentState; }
        WGPUBlendState getBlendState()        const { return mBlendState; }
        CameraState getCameraState()          const { return mCameraState; }

        const float* invView() const { return mInvView; }
        const float* invProj() const { return mInvProj; }

        inline void buildInvLookAt(float* out,
                               float ex, float ey, float ez,
                               float tx, float ty, float tz,
                               float upx = 0.0f, float upy = 1.0f, float upz = 0.0f)
            {
                // Same basis as buildLookAt
                float fx = tx - ex, fy = ty - ey, fz = tz - ez;
                const float fl = 1.0f / sqrtf(fx*fx + fy*fy + fz*fz);
                fx *= fl; fy *= fl; fz *= fl;

                // right = normalize(cross(f, up))
                float rx = fy*upz - fz*upy;
                float ry = fz*upx - fx*upz;
                float rz = fx*upy - fy*upx;
                const float rl = 1.0f / sqrtf(rx*rx + ry*ry + rz*rz);
                rx *= rl; ry *= rl; rz *= rl;

                // up = cross(right, f)
                const float ux = ry*fz - rz*fy;
                const float uy = rz*fx - rx*fz;
                const float uz = rx*fy - ry*fx;

                // Camera-to-world: rotation transposed relative to the view
                // matrix, eye position as the translation column.
                out[0]  = rx;   out[1]  = ry;   out[2]  = rz;   out[3]  = 0.0f;  // col 0: right
                out[4]  = ux;   out[5]  = uy;   out[6]  = uz;   out[7]  = 0.0f;  // col 1: up
                out[8]  = -fx;  out[9]  = -fy;  out[10] = -fz;  out[11] = 0.0f;  // col 2: -forward
                out[12] = ex;   out[13] = ey;   out[14] = ez;   out[15] = 1.0f;  // col 3: eye
            }

        inline void buildInvPerspective(float* out,
                                    float fovY, float aspect,
                                    float nearZ, float farZ)
            {
                // Forward matrix (WebGPU 0..1 depth, -1 in the w row):
                //   sx = 1 / (aspect * tan(fovY/2))
                //   sy = 1 / tan(fovY/2)
                //   A  = farZ / (nearZ - farZ)
                //   B  = (farZ * nearZ) / (nearZ - farZ)
                const float t  = tanf(fovY * 0.5f);
                const float A  = farZ / (nearZ - farZ);
                const float B  = (farZ * nearZ) / (nearZ - farZ);

                for (int i = 0; i < 16; ++i) out[i] = 0.0f;

                out[0]  = aspect * t;   // 1/sx
                out[5]  = t;            // 1/sy
                out[11] = 1.0f / B;
                out[14] = -1.0f;
                out[15] = A / B;
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

        float                               mSliderValues[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        // float                               mDepthValue      = 0.0f;

        struct SliderMesh {
            WGPUBuffer vertexBuffer = nullptr;
            WGPUBuffer indexBuffer  = nullptr;
            uint32_t   indexCount   = 0;
            uint32_t   materialId   = 0;
        };

        std::vector<SliderMesh>             mSliderMeshes;
        float                               mSliderPos[3]    = { 0.5f, 0.0f, 0.2f };
        static constexpr float              kSpineMinY       = -0.15f;
        static constexpr float              kSpineMaxY       =  0.25f;
        static constexpr float              kIndicatorHalfY  =  0.025f;
        //Plane
        // WGPUBuffer                          mPlaneVertexBuffer   = nullptr;
        // WGPUBuffer                          mPlaneIndexBuffer    = nullptr;
        // uint32_t                            mPlaneIndexCount     = 0;
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
        //Glyph
        WGPUBuffer                          mGlyphVertexBuffer   = nullptr;
        WGPUBuffer                          mGlyphIndexBuffer    = nullptr;
        uint32_t                            mGlyphIndexCount     = 0;

        AnimatedLogo mLogo;

        //Camera
        CameraState mCameraState;
        DragState mDrag;
        float mView[16]{}, mProj[16]{};
        float mInvView[16]{}, mInvProj[16]{};

        mutable double mRed = {};
        double mGreen = {};
        double mBlue = {};

        const std::vector<AnimatedSlider>* mSliderList = nullptr;  // borrowed from SliderManager

        const AnimatedSlider* findSlider(const juce::ParameterID& id) const
        {
            if (!mSliderList) return nullptr;
            for (const auto& s : *mSliderList)
                if (s.paramID.getParamID() == id.getParamID())
                    return &s;
            return nullptr;
        }


};


#endif //ANIMATEDNOISE_SCENE_H