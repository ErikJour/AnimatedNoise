//
// Created by Erik Jourgensen on 5/12/26.
//

#include "Scene.h"
#include <cmath>
#include <components/slider/sliderCatalog.h>

//================================================================================================
Scene::Scene() = default;

Scene::~Scene() = default;

//================================================================================================
void Scene::init(const WGPUDevice device, WGPUQueue queue) { mDevice = device; mQueue  = queue; }
void Scene::setSurface(WGPUSurface surface) { mSurface = surface; }
void Scene::setSurfaceSize(const uint32_t width, const uint32_t height) { mWidth = width; mHeight = height; }
void Scene::setShaderModule(const WGPUShaderModule shaderModule) { mShaderModule = shaderModule; }
void Scene::setPipelineDesc(WGPURenderPipelineDescriptor pipelineDesc) { mPipelineDesc = pipelineDesc; }

//================================================================================================
bool Scene::createShader()
{
#ifdef DEBUG
    mShaderPaths = getShaderPaths();
    mLastShaderWriteTime = latestWriteTime(mShaderPaths);
    mShaderModule        = ResourceManager::loadShaderModules(mShaderPaths, mDevice);
#else
    WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
    shaderCodeDesc.chain.next  = nullptr;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    shaderCodeDesc.code        = { shaderSource, strlen(shaderSource) };

    WGPUShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain     = &shaderCodeDesc.chain;
    mShaderModule              = wgpuDeviceCreateShaderModule(mDevice, &shaderDesc);
#endif

    if (!mShaderModule) {
        std::cerr << "Failed to create shader module." << std::endl;
        return false;
    }
    return true;
}

void Scene::terminate()
{
    mLogo.terminate();
    if (mDepthTextureView)              { wgpuTextureViewRelease(mDepthTextureView); mDepthTextureView = nullptr; }
    if (mDepthTexture)                  { wgpuTextureDestroy(mDepthTexture); wgpuTextureRelease(mDepthTexture); mDepthTexture = nullptr; }
    if (mSkylightVertexBuffer)          { wgpuBufferRelease(mSkylightVertexBuffer); mSkylightVertexBuffer = nullptr; }
    if (mSkylightIndexBuffer)           { wgpuBufferRelease(mSkylightIndexBuffer); mSkylightIndexBuffer = nullptr; }
    if (mSphereVertexBuffer)            { wgpuBufferRelease(mSphereVertexBuffer); mSphereVertexBuffer = nullptr; }
    if (mSphereIndexBuffer)             { wgpuBufferRelease(mSphereIndexBuffer); mSphereIndexBuffer = nullptr; }
    if (mFloorVertexBuffer)             { wgpuBufferRelease(mFloorVertexBuffer); mFloorVertexBuffer = nullptr; }
    if (mFloorIndexBuffer)              { wgpuBufferRelease(mFloorIndexBuffer); mFloorIndexBuffer = nullptr; }
    for (auto& m : mSliderMeshes)
    {
        if (m.vertexBuffer) { wgpuBufferRelease(m.vertexBuffer); m.vertexBuffer = nullptr; }
        if (m.indexBuffer)  { wgpuBufferRelease(m.indexBuffer);  m.indexBuffer  = nullptr; }
    }
    if (mParticleQuadBuffer)            { wgpuBufferRelease(mParticleQuadBuffer); mParticleQuadBuffer = nullptr; }
    if (mParticleDataBuffer)            { wgpuBufferRelease(mParticleDataBuffer); mParticleDataBuffer = nullptr; }
    if (mParticlePipeline)              { wgpuRenderPipelineRelease(mParticlePipeline); mParticlePipeline = nullptr; }
    if (mBindGroup)                     { wgpuBindGroupRelease(mBindGroup); mBindGroup = nullptr; }
    if (mUniformBuffer)                 { wgpuBufferRelease(mUniformBuffer); mUniformBuffer = nullptr; }
    if (mPipeline)                      { wgpuRenderPipelineRelease(mPipeline); mPipeline = nullptr; }
    if (mSurface)                       { wgpuSurfaceUnconfigure(mSurface); wgpuSurfaceRelease(mSurface); mSurface = nullptr; }
}

void Scene::renderFrame(const float currentTime)
{
    #ifdef DEBUG
        auto writeTime = latestWriteTime(mShaderPaths);
        if (writeTime != mLastShaderWriteTime) {
            mLastShaderWriteTime = writeTime;
            reloadShader();
            return;
        }
    #endif

        if (!mPipeline) return;
        if (!mSurface) return;

        WGPUSurfaceTexture surfaceTexture = {};
        wgpuSurfaceGetCurrentTexture(mSurface, &surfaceTexture);

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) return;
        WGPUTextureViewDescriptor viewDesc = {};
        viewDesc.format = mSurfaceFormat;
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.mipLevelCount = 1;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;
        setUniforms(mQueue, mUniformBuffer, currentTime);
        const WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDesc);
        wgpuTextureRelease(surfaceTexture.texture);
        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = WGPU_STR("Frame encoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, &encoderDesc);
        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view       = targetView;
        colorAttachment.loadOp     = WGPULoadOp_Clear;
        colorAttachment.storeOp    = WGPUStoreOp_Store;
        colorAttachment.clearValue = WGPUColor{ mRed, mGreen, mBlue, 1.0 };
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount     = 1;
        renderPassDesc.colorAttachments         = &colorAttachment;
        WGPURenderPassDepthStencilAttachment depthStencilAttachment = {};
        depthStencilAttachment.view             = mDepthTextureView;
        depthStencilAttachment.depthClearValue  = 1.0f;
        depthStencilAttachment.depthLoadOp      = WGPULoadOp_Clear;
        depthStencilAttachment.depthStoreOp     = WGPUStoreOp_Store;
        depthStencilAttachment.depthReadOnly    = false;
        depthStencilAttachment.stencilLoadOp    = WGPULoadOp_Undefined;
        depthStencilAttachment.stencilStoreOp   = WGPUStoreOp_Undefined;
        depthStencilAttachment.stencilReadOnly  = true;
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
        const WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        wgpuRenderPassEncoderSetPipeline(renderPass, mPipeline);
        //Floor
        setItemBuffers(mFloorVertexBuffer, mFloorIndexBuffer, mFloorIndexCount, MAT_FLOOR, renderPass);
        setItemBuffers(mSphereVertexBuffer, mSphereIndexBuffer, mSphereIndexCount, MAT_FLOOR, renderPass);
        //Skylight
        setItemBuffers(mSkylightVertexBuffer, mSkylightIndexBuffer, mSkylightIndexCount, MAT_FLOOR, renderPass);
        // Sliders
        for (const auto& m : mSliderMeshes)
            setItemBuffers(m.vertexBuffer, m.indexBuffer, m.indexCount, m.materialId, renderPass);
        // Particles
        if (mParticleQuadBuffer && mParticleDataBuffer && mParticleCount > 0)
        {
            wgpuRenderPassEncoderSetPipeline(renderPass, mParticlePipeline);
            const uint32_t offset = MAT_PARTICLES * mUniformStride;
            wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 1, &offset);
            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mParticleQuadBuffer, 0, wgpuBufferGetSize(mParticleQuadBuffer));
            wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, mParticleDataBuffer, 0, wgpuBufferGetSize(mParticleDataBuffer));
            wgpuRenderPassEncoderDraw(renderPass, 6, mParticleDrawCount, 0, 0);
            wgpuRenderPassEncoderSetPipeline(renderPass, mPipeline);
        }

        mLogo.render(renderPass);

        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);

        WGPUCommandBufferDescriptor cmdDesc = {};
        cmdDesc.label = WGPU_STR("Frame command buffer");
        const WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdDesc);
        wgpuCommandEncoderRelease(encoder);
        wgpuQueueSubmit(mQueue, 1, &command);
        wgpuCommandBufferRelease(command);
        wgpuTextureViewRelease(targetView);
        wgpuSurfacePresent(mSurface);
        wgpuDeviceTick(mDevice);
}

void Scene::setUniforms(WGPUQueue queue, const WGPUBuffer uniformBuffer, const float time)
{
    mUniforms.time         = time;
    mUniforms.frequency    = 10.0f;
    mUniforms.amplitude    = 0.5f;
    mUniforms.lightPos[0]  = 0.0f;
    mUniforms.lightPos[1]  = 0.35f;
    mUniforms.lightPos[2]  = 0.0f;
    mUniforms.aspectRatio  = static_cast<float>(mWidth) / static_cast<float>(mHeight);

    updateViewMatrix();

    const AnimatedSlider* noiseLevel    = findSlider(ParameterID::noiseLevel);
    const AnimatedSlider* combLevel     = findSlider(ParameterID::combLevel);
    const AnimatedSlider* lpgResonance  = findSlider(ParameterID::lpgResonance);

    const float resonanceVal    = lpgResonance ? lpgResonance->value : 0.0f;
    juce::ignoreUnused(resonanceVal);
    const float gainVal         = noiseLevel ? noiseLevel->value : 0.0f;
    const float morphVal        = combLevel  ? combLevel->value  : 0.0f;
    const bool  gainHeld        = noiseLevel ? noiseLevel->pressed : false;

    constexpr uint32_t ids[9] = {
                                    MAT_CAVE,
                                    MAT_GLOBAL_GAIN_SLIDER,
                                    MAT_COMB_AMT_SLIDER,
                                    MAT_PLANE,
                                    MAT_PARTICLES,
                                    MAT_FLOOR,
                                    MAT_SKYLIGHT,
                                    MAT_LPG_REZ_SLIDER,
                                    MAT_NOIS_DENS_SLIDER
                                };

    auto sliderForMaterial = [&](const uint32_t mat) -> const AnimatedSlider* {
        if (mSliderList)
            for (const auto& s : *mSliderList)
                if (s.materialId == mat) return &s;
        return nullptr;
    };

    for (const unsigned int id : ids)
    {
        mUniforms.materialId = id;

            std::memcpy(mUniforms.modelMatrix, kIdentity, sizeof(kIdentity));

            if (id == MAT_PARTICLES)
            {
                mUniforms.morph       = morphVal;
                mUniforms.sliderValue = gainVal;
                mUniforms.pressed     = gainHeld ? 1.0f : 0.0f;
                mUniforms.resonate     = resonanceVal;
            }
            else if (const AnimatedSlider* s = sliderForMaterial(id))
            {
                mUniforms.sliderValue = s->value;
                mUniforms.pressed     = s->pressed ? 1.0f : 0.0f;
            }
            else
            {
                mUniforms.sliderValue = 0.0f;
                mUniforms.pressed     = 0.0f;
            }


        if (const AnimatedSlider* dens = findSlider(ParameterID::noiseDensity))
            mParticleDrawCount = static_cast<uint32_t>(dens->value * MAX_PARTICLES - 100) + 100;

        wgpuQueueWriteBuffer(queue, uniformBuffer,
                             id * mUniformStride, &mUniforms, sizeof(MyUniforms));
    }
}

void Scene::ConfigureVertexLayout()
{
    mVertexAttribs[0].shaderLocation        = 0;
    mVertexAttribs[0].format                = WGPUVertexFormat_Float32x3;
    mVertexAttribs[0].offset                = 0;
    mVertexAttribs[1].shaderLocation        = 2;
    mVertexAttribs[1].format                = WGPUVertexFormat_Float32x3;
    mVertexAttribs[1].offset                = 3 * sizeof(float);
    mVertexAttribs[2].shaderLocation        = 1;
    mVertexAttribs[2].format                = WGPUVertexFormat_Float32x3;
    mVertexAttribs[2].offset                = 6 * sizeof(float);
    mVertexBufferLayouts.resize(1);
    mVertexBufferLayouts[0].attributeCount  = 3;
    mVertexBufferLayouts[0].attributes      = mVertexAttribs.data();
    mVertexBufferLayouts[0].arrayStride     = 9 * sizeof(float);
    mVertexBufferLayouts[0].stepMode        = WGPUVertexStepMode_Vertex;
    mPipelineDesc.vertex.bufferCount        = 1;
    mPipelineDesc.vertex.buffers            = mVertexBufferLayouts.data();
}

#ifdef DEBUG
void Scene::reloadShader()
{
    const WGPUShaderModule newModule = ResourceManager::loadShaderModules(mShaderPaths, mDevice);
    if (!newModule) {
        std::cerr << "Shader compile failed — keeping old pipeline." << std::endl;
        return;
    }
    wgpuShaderModuleRelease(mShaderModule);
    mShaderModule               = newModule;
    mPipelineDesc.vertex.module = mShaderModule;
    createPipeline();
    std::cout << "Shader reloaded." << std::endl;
}
#endif

bool Scene::createParticlePipeline()
{
    if (mParticlePipeline) { wgpuRenderPipelineRelease(mParticlePipeline); mParticlePipeline = nullptr; }

    mParticleBlendState.color.operation                = WGPUBlendOperation_Add;
    mParticleBlendState.color.srcFactor                = WGPUBlendFactor_SrcAlpha;
    mParticleBlendState.color.dstFactor                = WGPUBlendFactor_One;
    mParticleBlendState.alpha.operation                = WGPUBlendOperation_Add;
    mParticleBlendState.alpha.srcFactor                = WGPUBlendFactor_One;
    mParticleBlendState.alpha.dstFactor                = WGPUBlendFactor_One;
    mParticleColorTarget                               = mColorTarget;
    mParticleColorTarget.blend                         = &mParticleBlendState;
    mParticleVertexAttribs[0].shaderLocation           = 0;
    mParticleVertexAttribs[0].format                   = WGPUVertexFormat_Float32x2;
    mParticleVertexAttribs[0].offset                   = 0;
    mParticleVertexAttribs[1].shaderLocation           = 1;
    mParticleVertexAttribs[1].format                   = WGPUVertexFormat_Float32x2;
    mParticleVertexAttribs[1].offset                   = 2 * sizeof(float);
    mParticleVertexAttribs[2].shaderLocation           = 2;
    mParticleVertexAttribs[2].format                   = WGPUVertexFormat_Float32x4;
    mParticleVertexAttribs[2].offset                   = 0;
    mParticleVertexAttribs[3].shaderLocation           = 3;
    mParticleVertexAttribs[3].format                   = WGPUVertexFormat_Float32x4;
    mParticleVertexAttribs[3].offset                   = 4 * sizeof(float);
    mParticleVertexAttribs[4].shaderLocation           = 4;
    mParticleVertexAttribs[4].format                   = WGPUVertexFormat_Float32x4;
    mParticleVertexAttribs[4].offset                   = 8 * sizeof(float);
    mParticleVertexBufferLayouts.resize(2);
    mParticleVertexBufferLayouts[0].attributeCount     = 2;
    mParticleVertexBufferLayouts[0].attributes         = &mParticleVertexAttribs[0];
    mParticleVertexBufferLayouts[0].arrayStride        = sizeof(QuadVertex);
    mParticleVertexBufferLayouts[0].stepMode           = WGPUVertexStepMode_Vertex;
    mParticleVertexBufferLayouts[1].attributeCount     = 3;
    mParticleVertexBufferLayouts[1].attributes         = &mParticleVertexAttribs[2];
    mParticleVertexBufferLayouts[1].arrayStride        = sizeof(ParticleData);
    mParticleVertexBufferLayouts[1].stepMode           = WGPUVertexStepMode_Instance;
    mParticlePipelineDesc                              = mPipelineDesc;
    mParticlePipelineDesc.vertex.module                = mShaderModule;
    mParticlePipelineDesc.vertex.entryPoint            = WGPU_STR("vs_particle_world");
    mParticlePipelineDesc.vertex.bufferCount           = 2;
    mParticlePipelineDesc.vertex.buffers               = mParticleVertexBufferLayouts.data();
    mParticlePipelineDesc.vertex.constantCount         = 0;
    mParticlePipelineDesc.vertex.constants             = nullptr;
    mParticleFragmentState.module                      = mShaderModule;
    mParticleFragmentState.entryPoint                  = WGPU_STR("fs_particle");
    mParticleFragmentState.targetCount                 = 1;
    mParticleFragmentState.targets =                   &mParticleColorTarget;
    mParticleFragmentState.constants                   = nullptr;
    mParticlePipelineDesc.fragment                     = &mParticleFragmentState;
    mParticleDepthStencil                              = *mPipelineDesc.depthStencil;
    mParticleDepthStencil.depthWriteEnabled            = WGPUOptionalBool_False;
    mParticlePipelineDesc.depthStencil                 = &mParticleDepthStencil;

    mParticlePipeline = wgpuDeviceCreateRenderPipeline(mDevice, &mParticlePipelineDesc);
    if (!mParticlePipeline) {
        std::cerr << "Failed to create particle render pipeline." << std::endl;
        return false;
    }
    return true;
}

void Scene::initializeScene()
{
    initializeFloor();
    initializeSphere();
    //================================================
    mSliderMeshes.clear();
    mSliderMeshes.reserve(sliderDefinitions().size());
    for (const auto& def : sliderDefinitions())
    {
        constexpr float kSliderWallRadius = 0.9f;
        SliderMesh mesh;
        mesh.materialId = def.materialId;

        InitializeSlider(mesh.indexCount, mesh.vertexBuffer, mesh.indexBuffer,
                         kSliderWallRadius, def.angle);

        mSliderMeshes.push_back(mesh);
    }
    //================================================
    initializeParticles();
}


bool Scene::createPipeline()
{
    if (mBindGroup)    { wgpuBindGroupRelease(mBindGroup);    mBindGroup     = nullptr; }
    if (mUniformBuffer){ wgpuBufferRelease(mUniformBuffer);   mUniformBuffer = nullptr; }
    if (mPipeline)     { wgpuRenderPipelineRelease(mPipeline); mPipeline     = nullptr; }

    static constexpr uint32_t kAlignment = 256;
    mUniformStride = (sizeof(MyUniforms) + kAlignment - 1) & ~(kAlignment - 1);

    WGPUBindGroupLayoutEntry bglEntry  = {};
    bglEntry.binding                   = 0;
    bglEntry.visibility                = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
    bglEntry.buffer.type               = WGPUBufferBindingType_Uniform;
    bglEntry.buffer.hasDynamicOffset   = true;
    bglEntry.buffer.minBindingSize     = sizeof(MyUniforms);

    WGPUBindGroupLayoutDescriptor bglDesc = {};
    bglDesc.entryCount                    = 1;
    bglDesc.entries                       = &bglEntry;
    WGPUBindGroupLayout bgl = wgpuDeviceCreateBindGroupLayout(mDevice, &bglDesc);

    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.bindGroupLayoutCount         = 1;
    layoutDesc.bindGroupLayouts             = &bgl;
    if (mPipelineDesc.layout) {
        wgpuPipelineLayoutRelease(mPipelineDesc.layout);
        mPipelineDesc.layout = nullptr;
    }
    mPipelineDesc.layout = wgpuDeviceCreatePipelineLayout(mDevice, &layoutDesc);

    mColorTarget.format        = mSurfaceFormat;
    mColorTarget.blend         = &mBlendState;
    mColorTarget.writeMask     = WGPUColorWriteMask_All;
    mPipelineDesc.vertex.module = mShaderModule;
    mFragmentState.module      = mShaderModule;
    mFragmentState.entryPoint  = WGPU_STR("fs_main");
    mFragmentState.targetCount = 1;
    mFragmentState.targets     = &mColorTarget;
    mFragmentState.constants   = nullptr;
    mPipelineDesc.fragment     = &mFragmentState;

    mPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &mPipelineDesc);
    if (!mPipeline) {
        std::cerr << "Failed to create render pipeline." << std::endl;
        return false;
    }

    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size                 = 9 * mUniformStride;  // one slot per material
    bufferDesc.usage                = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    mUniformBuffer                  = wgpuDeviceCreateBuffer(mDevice, &bufferDesc);

    WGPUBindGroupEntry entry       = {};
    entry.binding                  = 0;
    entry.buffer                   = mUniformBuffer;
    entry.offset                   = 0;
    entry.size                     = sizeof(MyUniforms);

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout                  = bgl;
    bgDesc.entryCount              = 1;
    bgDesc.entries                 = &entry;
    mBindGroup                     = wgpuDeviceCreateBindGroup(mDevice, &bgDesc);

    updateDepthTexture(mWidth, mHeight);

    wgpuBindGroupLayoutRelease(bgl);
    if (!createParticlePipeline()) return false;
    mLogo.init(mDevice, mQueue);
    if (!mLogo.initialize(mSurfaceFormat, *mPipelineDesc.depthStencil)) return false;
    return true;
}

void Scene::updateDepthTexture(const uint32_t width, const uint32_t height)
{
    mWidth = width;
    mHeight = height;
    if (mDepthTextureView) { wgpuTextureViewRelease(mDepthTextureView); mDepthTextureView = nullptr; }
    if (mDepthTexture)     { wgpuTextureDestroy(mDepthTexture); wgpuTextureRelease(mDepthTexture); mDepthTexture = nullptr; }

    WGPUTextureDescriptor depthDesc{};
    depthDesc.usage         = WGPUTextureUsage_RenderAttachment;
    depthDesc.dimension     = WGPUTextureDimension_2D;
    depthDesc.size          = { mWidth, mHeight, 1 };
    depthDesc.format        = WGPUTextureFormat_Depth24Plus;
    depthDesc.mipLevelCount = 1;
    depthDesc.sampleCount   = 1;
    mDepthTexture           = wgpuDeviceCreateTexture(mDevice, &depthDesc);

    WGPUTextureViewDescriptor dvDesc{};
    dvDesc.format          = WGPUTextureFormat_Depth24Plus;
    dvDesc.dimension       = WGPUTextureViewDimension_2D;
    dvDesc.mipLevelCount   = 1;
    dvDesc.arrayLayerCount = 1;
    dvDesc.aspect          = WGPUTextureAspect_DepthOnly;
    mDepthTextureView      = wgpuTextureCreateView(mDepthTexture, &dvDesc);
}

void Scene::initializeFloor()
{
    std::vector<FloorVertex> vertices;
    std::vector<FloorIndex>  indices;

    CircularFloor::buildCircle(vertices, indices, 0.95f, 64);  // was 1.0f

    mFloorIndexCount = static_cast<uint32_t>(indices.size());

    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = vertices.size() * sizeof(FloorVertex);
    mFloorVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mFloorVertexBuffer, 0, vertices.data(), bd.size);

    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bd.size  = (indices.size() * sizeof(FloorIndex) + 3) & ~3ULL;
    mFloorIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mFloorIndexBuffer, 0, indices.data(), bd.size);
}

void Scene::initializeSphere()
{
    std::cout << "Initializing Sphere" << std::endl;
    std::vector<SphereVertex> vertices;
    std::vector<SphereIndex>  indices;

    SphereGeometry::buildSphere(vertices, indices);

    mSphereIndexCount = static_cast<uint32_t>(indices.size());

    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = vertices.size() * sizeof(SphereVertex);
    mSphereVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mSphereVertexBuffer, 0, vertices.data(), bd.size);

    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bd.size  = (indices.size() * sizeof(FloorIndex) + 3) & ~3ULL;
    mSphereIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mSphereIndexBuffer, 0, indices.data(), bd.size);
}

void Scene::initializeSkylight()
{
    std::vector<SkylightVertex> vertices;
    std::vector<SkylightIndex>  indices;

    Skylight::buildSkylight(vertices, indices, 0.15f, 0.75, 64);

    mSkylightIndexCount = static_cast<uint32_t>(indices.size());

    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = vertices.size() * sizeof(SkylightVertex);
    mSkylightVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mSkylightVertexBuffer, 0, vertices.data(), bd.size);

    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bd.size  = (indices.size() * sizeof(SkylightIndex) + 3) & ~3ULL;
    mSkylightIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mSkylightIndexBuffer, 0, indices.data(), bd.size);
}

void Scene::InitializeSlider(uint32_t& indexCount, WGPUBuffer& vertexBuffer, WGPUBuffer& indexBuffer, const float wallRadius = 0.9f, const float angle = 2.975f) const
{
    std::vector<SliderVertex> verts;
    std::vector<SliderIndex>  indices;
    buildSliderGeometry(verts, indices, wallRadius, angle);
    indexCount = static_cast<uint32_t>(indices.size());
    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size = verts.size() * sizeof(SliderVertex);
    vertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, vertexBuffer, 0, verts.data(), bd.size);
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bd.size  = (indices.size() * sizeof(SliderIndex) + 3) & ~3ULL;
    indexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, indexBuffer, 0, indices.data(), bd.size);
}

void Scene::initializeParticles()
{
    std::vector<QuadVertex>   quadVerts;
    std::vector<ParticleData> particles;

    ParticleSystem::buildQuad(quadVerts);

    constexpr float particleSpread = 0.4f;
    constexpr float particleSize = 0.015f;
    ParticleSystem::initParticles(particles, MAX_PARTICLES, particleSpread, particleSize);

    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = quadVerts.size() * sizeof(QuadVertex);
    mParticleQuadBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mParticleQuadBuffer, 0, quadVerts.data(), bd.size);

    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = particles.size() * sizeof(ParticleData);
    mParticleDataBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mParticleDataBuffer, 0, particles.data(), bd.size);

    mParticleCount = static_cast<uint32_t>(particles.size());
    mParticleDrawCount = mParticleCount;
}

//==========================================================================
void Scene::updateViewMatrix()
{
    const float yaw          = mCameraState.angleX;
    const float cameraX      = mCameraState.posX;
    constexpr float cameraY  = CameraState::eyeY;
    const float cameraZ      = mCameraState.posZ;

    // Look target one unit ahead in the yaw direction
    const float tx = cameraX + sinf(yaw);
    const float tz = cameraZ - cosf(yaw);
    constexpr float near = 0.1f;
    constexpr float far = 8.0f;

    float view[16], proj[16];
    buildLookAt(view, cameraX, cameraY, cameraZ, tx, cameraY, tz);
    buildPerspective(proj, 1.047f, mUniforms.aspectRatio, near, far);
    mulMat4(mUniforms.viewProjMatrix, proj, view);
    std::memcpy(mUniforms.projMatrix, proj, sizeof(proj));
}

void Scene::onMouseMove(const float xpos, const float /*ypos*/)
{
    if (mDrag.active)
    {
        mCameraState.angleX = mDrag.startAngleX + (xpos - mDrag.startMouseX) * mDrag.sensitivity;
        updateViewMatrix();
    }
}

void Scene::onMouseButton(const int button, const bool isPressed, const float xpos, float /*ypos*/)
{
    if (button == 0)
    {
        if (isPressed)
        {
            mDrag.active      = true;
            mDrag.startMouseX = xpos;
            mDrag.startAngleX = mCameraState.angleX;
        }
        else
        {
            mDrag.active = false;
        }
    }
}

void Scene::onScroll(const float deltaX, const float deltaY)
{
    const float yaw     = mCameraState.angleX;
    const float speed   = mDrag.scrollSensitivity;

    mCameraState.posX +=  sinf(yaw) * deltaY * speed;
    mCameraState.posZ += -cosf(yaw) * deltaY * speed;
    mCameraState.angleX += deltaX * mDrag.turnSensitivity * speed;
    const float r = sqrtf(mCameraState.posX * mCameraState.posX +
                          mCameraState.posZ * mCameraState.posZ);
    if (r > CameraState::kWallRadius)
    {
        const float inv = CameraState::kWallRadius / r;
        mCameraState.posX *= inv;
        mCameraState.posZ *= inv;
    }
    updateViewMatrix();
}

void Scene::setSurfaceFormat(const WGPUTextureFormat format) { mSurfaceFormat = format; }

void Scene::setCameraState(const CameraState& s)             { mCameraState = s; updateViewMatrix(); }

float Scene::getSliderValue(const int index) const           { return mSliderValues[index]; }

float Scene::sliderTopFraction() const                       { return (1.0f - (kSpineMaxY + mSliderPos[1])) * 0.5f; }

float Scene::sliderBottomFraction() const                    { return (1.0f - (kSpineMinY + mSliderPos[1])) * 0.5f; }

float Scene::sliderXFraction() const                         { return (mSliderPos[0] + 1.0f) * 0.5f; }

float Scene::indicatorHalfFraction()                         { return kIndicatorHalfY * 0.5f; }