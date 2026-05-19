//
// Created by Erik Jourgensen on 5/12/26.
//

#include "Scene.h"

#include "skylight.h"


Scene::Scene() = default;

Scene::~Scene() = default;

void Scene::init(const WGPUDevice device, WGPUQueue queue)
{
    mDevice = device;
    mQueue  = queue;
}

//==============================================
//Setters
//==============================================
void Scene::setSurface(WGPUSurface surface) { mSurface = surface; }
void Scene::setShaderModule(const WGPUShaderModule shaderModule) { mShaderModule = shaderModule; }
void Scene::setPipelineDesc(WGPURenderPipelineDescriptor pipelineDesc) { mPipelineDesc = pipelineDesc; }
void Scene::setWindowColor() { mRed    = 0.2; mGreen  = 0.25; mBlue   = 0.2; }

bool Scene::createShader()
{
#ifdef DEBUG
    const std::string dir = DEBUG_SHADER_DIR;
    mShaderPaths = {
        dir + "/common.wgsl",
        dir + "/lighting.wgsl",
        dir + "/mat_cave.wgsl",
        dir + "/mat_slider.wgsl",
        dir + "/mat_plane.wgsl",
        dir + "/mat_particle.wgsl",
        dir + "/mat_floor.wgsl",
        dir + "/mat_skylight.wgsl",
        dir + "/vs_main.wgsl",
        dir + "/fs_main.wgsl",
    };
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
    if (mDepthTextureView)       { wgpuTextureViewRelease(mDepthTextureView); mDepthTextureView = nullptr; }
    if (mDepthTexture)           { wgpuTextureDestroy(mDepthTexture); wgpuTextureRelease(mDepthTexture); mDepthTexture = nullptr; }
    if (mSkylightVertexBuffer)   { wgpuBufferRelease(mSkylightVertexBuffer); mSkylightVertexBuffer = nullptr; }
    if (mSkylightIndexBuffer)    { wgpuBufferRelease(mSkylightIndexBuffer); mSkylightIndexBuffer = nullptr; }
    if (mFloorVertexBuffer)      { wgpuBufferRelease(mFloorVertexBuffer); mFloorVertexBuffer = nullptr; }
    if (mFloorIndexBuffer)       { wgpuBufferRelease(mFloorIndexBuffer); mFloorIndexBuffer = nullptr; }
    if (mBindGroup)              { wgpuBindGroupRelease(mBindGroup); mBindGroup = nullptr; }
    if (mUniformBuffer)          { wgpuBufferRelease(mUniformBuffer); mUniformBuffer = nullptr; }
    if (mPipeline)               { wgpuRenderPipelineRelease(mPipeline); mPipeline = nullptr; }
    if (mSurface)                { wgpuSurfaceUnconfigure(mSurface); wgpuSurfaceRelease(mSurface); mSurface = nullptr; }
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
        //=====================================
        //Push into buffer / fill buffer
        //=====================================
        setUniforms(mQueue, mUniformBuffer, currentTime);
        const WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDesc);
        wgpuTextureRelease(surfaceTexture.texture);

        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = WGPU_STR("Frame encoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, &encoderDesc);
        //======================================================
        //Color Attachment
        //======================================================
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
        renderPassDesc.depthStencilAttachment = &depthStencilAttachment; // was nullptr

        const WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderSetPipeline(renderPass, mPipeline);
        //Floor
        setItemBuffers(mFloorVertexBuffer, mFloorIndexBuffer, mFloorIndexCount, MAT_FLOOR, renderPass);
        //Skylight
        setItemBuffers(mSkylightVertexBuffer, mSkylightIndexBuffer, mSkylightIndexCount, MAT_SKYLIGHT, renderPass);
        //Cave
        setItemBuffers(mCaveVertexBuffer, mCaveIndexBuffer, mCaveIndexCount, MAT_CAVE, renderPass);
        // Slider
        setItemBuffers(mSliderVertexBuffer, mSliderIndexBuffer, mSliderIndexCount, MAT_SLIDER, renderPass);
        //Plane
        setItemBuffers(mPlaneVertexBuffer, mPlaneIndexBuffer, mPlaneIndexCount, MAT_PLANE, renderPass);

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

        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);

        WGPUCommandBufferDescriptor cmdDesc = {};
        cmdDesc.label = WGPU_STR("Frame command buffer");
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdDesc);
        wgpuCommandEncoderRelease(encoder);
        //==============================================
        //Process the frame
        //==============================================
        wgpuQueueSubmit(mQueue, 1, &command);
        wgpuCommandBufferRelease(command);
        wgpuTextureViewRelease(targetView);
        wgpuSurfacePresent(mSurface);
        wgpuDeviceTick(mDevice);
}

void Scene::setUniforms(WGPUQueue queue, const WGPUBuffer uniformBuffer, const float time)
{

    mUniforms.time        = time;
    mUniforms.frequency   = 10.0f;
    mUniforms.amplitude   = 0.5f;
    mUniforms.sliderValue  = mSliderValue;
    mUniforms.lightPos[0]  = 0.0f;
    mUniforms.lightPos[1]  = 0.75f;
    mUniforms.lightPos[2]  = 0.35f;
    mUniforms.sliderPos[0] = mSliderPos[0];
    mUniforms.sliderPos[1] = mSliderPos[1];
    mUniforms.sliderPos[2] = mSliderPos[2];

    constexpr uint32_t ids[6] = { MAT_CAVE, MAT_SLIDER, MAT_PLANE, MAT_PARTICLES, MAT_FLOOR, MAT_SKYLIGHT };
    std::memcpy(mUniforms.modelMatrix, kIdentity, sizeof(kIdentity));

    for (uint32_t i = 0; i < 6; ++i) {
        mUniforms.materialId = ids[i];
        wgpuQueueWriteBuffer(queue, uniformBuffer, ids[i] * mUniformStride, &mUniforms, sizeof(MyUniforms));
    }

    static constexpr float kFloorMatrix[16] = {
        1.0f,  0.0f,   0.0f,  0.0f,
        0.0f,  0.866f, 0.5f,  0.0f,
        0.0f, -0.5f,   0.966f,0.0f,
        0.0f, -0.3f,   0.5f,  1.0f
    };
    static constexpr float kSkylightMatrix[16] = {
        1.0f,  0.0f,   0.0f,  0.0f,
        0.0f,  0.366f, 0.5f,  0.0f,
        0.0f, -0.5f,   0.366f,0.0f,
        0.0f,  0.3f,   0.5f,  1.0f
    };
    MyUniforms floorUniforms = mUniforms;
    floorUniforms.materialId = MAT_FLOOR;
    std::memcpy(floorUniforms.modelMatrix, kFloorMatrix, sizeof(kFloorMatrix));
    wgpuQueueWriteBuffer(queue, uniformBuffer, MAT_FLOOR * mUniformStride,
                         &floorUniforms, sizeof(MyUniforms));

    MyUniforms skylightUniforms = mUniforms;
    skylightUniforms.materialId = MAT_SKYLIGHT;
    std::memcpy(skylightUniforms.modelMatrix, kSkylightMatrix, sizeof(kSkylightMatrix));
    wgpuQueueWriteBuffer(queue, uniformBuffer, MAT_SKYLIGHT * mUniformStride,
                         &skylightUniforms, sizeof(MyUniforms));
}

void Scene::ConfigureVertexLayout()
{
    // Attribute 0 — Position (location 0)
    mVertexAttribs[0].shaderLocation = 0;
    mVertexAttribs[0].format         = WGPUVertexFormat_Float32x3;
    mVertexAttribs[0].offset         = 0;
    // Attribute 1 — Normal (location 2)
    mVertexAttribs[1].shaderLocation = 2;
    mVertexAttribs[1].format         = WGPUVertexFormat_Float32x3;
    mVertexAttribs[1].offset         = 3 * sizeof(float);
    // Attribute 2 — Color (location 1)
    mVertexAttribs[2].shaderLocation = 1;
    mVertexAttribs[2].format         = WGPUVertexFormat_Float32x3;
    mVertexAttribs[2].offset         = 6 * sizeof(float);

    mVertexBufferLayouts.resize(1);
    mVertexBufferLayouts[0].attributeCount = 3;
    mVertexBufferLayouts[0].attributes     = mVertexAttribs.data();
    mVertexBufferLayouts[0].arrayStride    = 9 * sizeof(float);
    mVertexBufferLayouts[0].stepMode       = WGPUVertexStepMode_Vertex;

    mPipelineDesc.vertex.bufferCount = 1;
    mPipelineDesc.vertex.buffers     = mVertexBufferLayouts.data();
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

    // ── Slot 0: QuadVertex, per-vertex ──────────────────────────
    // @location(0) cornerOffset : vec2f   offset 0
    // @location(1) uv           : vec2f   offset 8
    mParticleVertexAttribs[0].shaderLocation = 0;
    mParticleVertexAttribs[0].format         = WGPUVertexFormat_Float32x2;
    mParticleVertexAttribs[0].offset         = 0;

    mParticleVertexAttribs[1].shaderLocation = 1;
    mParticleVertexAttribs[1].format         = WGPUVertexFormat_Float32x2;
    mParticleVertexAttribs[1].offset         = 2 * sizeof(float);

    // ── Slot 1: ParticleData, per-instance ──────────────────────
    // @location(2) pos_size  : vec4f   offset  0  (xyz = world pos, w = size)
    // @location(3) color     : vec4f   offset 16
    // @location(4) life_vel  : vec4f   offset 32
    mParticleVertexAttribs[2].shaderLocation = 2;
    mParticleVertexAttribs[2].format         = WGPUVertexFormat_Float32x4;
    mParticleVertexAttribs[2].offset         = 0;

    mParticleVertexAttribs[3].shaderLocation = 3;
    mParticleVertexAttribs[3].format         = WGPUVertexFormat_Float32x4;
    mParticleVertexAttribs[3].offset         = 4 * sizeof(float);

    mParticleVertexAttribs[4].shaderLocation = 4;
    mParticleVertexAttribs[4].format         = WGPUVertexFormat_Float32x4;
    mParticleVertexAttribs[4].offset         = 8 * sizeof(float);

    mParticleVertexBufferLayouts.resize(2);

    mParticleVertexBufferLayouts[0].attributeCount = 2;
    mParticleVertexBufferLayouts[0].attributes     = &mParticleVertexAttribs[0];
    mParticleVertexBufferLayouts[0].arrayStride    = sizeof(QuadVertex);
    mParticleVertexBufferLayouts[0].stepMode       = WGPUVertexStepMode_Vertex;

    mParticleVertexBufferLayouts[1].attributeCount = 3;
    mParticleVertexBufferLayouts[1].attributes     = &mParticleVertexAttribs[2];
    mParticleVertexBufferLayouts[1].arrayStride    = sizeof(ParticleData);
    mParticleVertexBufferLayouts[1].stepMode       = WGPUVertexStepMode_Instance;

    // ── Pipeline descriptor ─────────────────────────────────────
    // Copy the main desc as a base — shares layout, blend, depth, multisample
    mParticlePipelineDesc                              = mPipelineDesc;
    mParticlePipelineDesc.vertex.module                = mShaderModule;
    mParticlePipelineDesc.vertex.entryPoint            = WGPU_STR("vs_particle");
    mParticlePipelineDesc.vertex.bufferCount           = 2;
    mParticlePipelineDesc.vertex.buffers               = mParticleVertexBufferLayouts.data();
    mParticlePipelineDesc.vertex.constantCount         = 0;
    mParticlePipelineDesc.vertex.constants             = nullptr;
    mParticleFragmentState.module                      = mShaderModule;
    mParticleFragmentState.entryPoint                  = WGPU_STR("fs_particle");
    mParticleFragmentState.targetCount                 = 1;
    mParticleFragmentState.targets                     = &mColorTarget;
    mParticleFragmentState.constants                   = nullptr;

    // Reuse the same fragment state — swap entry point if you want a dedicated one
    mParticlePipelineDesc.fragment = &mParticleFragmentState;  // separate state

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
    initializeSkylight();
    // initializePlane();
    // InitializeSlider();
    // initializeParticles();
    // InitializeProceduralCave();

}

void Scene::onResize()
{
    // Terminate in reverse order
    // terminateDepthBuffer();
    // terminateSwapChain();
    //
    // Re-init
    // initSwapChain();
    // initDepthBuffer();
}


bool Scene::createPipeline()
{
    if (mBindGroup)    { wgpuBindGroupRelease(mBindGroup);    mBindGroup     = nullptr; }
    if (mUniformBuffer){ wgpuBufferRelease(mUniformBuffer);   mUniformBuffer = nullptr; }
    if (mPipeline)     { wgpuRenderPipelineRelease(mPipeline); mPipeline     = nullptr; }

    static constexpr uint32_t kAlignment = 256; // minUniformBufferOffsetAlignment
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
    mPipelineDesc.vertex.module = mShaderModule;  // ← add this
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
    bufferDesc.size                 = 6 * mUniformStride;  // one slot per material
    bufferDesc.usage                = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    mUniformBuffer                  = wgpuDeviceCreateBuffer(mDevice, &bufferDesc);

    WGPUBindGroupEntry entry = {};
    entry.binding            = 0;
    entry.buffer             = mUniformBuffer;
    entry.offset             = 0;
    entry.size               = sizeof(MyUniforms);

    WGPUBindGroupDescriptor bgDesc = {};
    bgDesc.layout                  = bgl;
    bgDesc.entryCount              = 1;
    bgDesc.entries                 = &entry;
    mBindGroup                     = wgpuDeviceCreateBindGroup(mDevice, &bgDesc);

    // Depth texture
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

    wgpuBindGroupLayoutRelease(bgl);
    if (!createParticlePipeline()) return false;
    return true;
}

void Scene::InitializeProceduralCave()
{
    std::vector<Vertex> verts;
    std::vector<Index>  indices;
    perlinCave::buildCaveGeometry(verts, indices);
    mCaveIndexCount = static_cast<uint32_t>(indices.size());
    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = verts.size() * sizeof(Vertex);
    mCaveVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mCaveVertexBuffer, 0, verts.data(), bd.size);
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bd.size  = (indices.size() * sizeof(Index) + 3) & ~3ULL;
    mCaveIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mCaveIndexBuffer, 0, indices.data(), bd.size);
}

void Scene::initializeFloor()
{
    std::cout << "Initialize Floor" << std::endl;
    std::vector<FloorVertex> vertices;
    std::vector<FloorIndex>  indices;

    CircularFloor::buildCircle(vertices, indices, 0.5f, 64);  // was 1.0f

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

void Scene::initializeSkylight()
{
    std::cout << "Initialize Skylight" << std::endl;
    std::vector<SkylightVertex> vertices;
    std::vector<SkylightIndex>  indices;

    Skylight::buildSkylight(vertices, indices, 0.15f, 64);  // was 1.0f

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

void Scene::InitializeSlider()
{
    std::vector<Vertex> verts;
    std::vector<Index>  indices;
    buildSliderGeometry(verts, indices);

    mSliderIndexCount = static_cast<uint32_t>(indices.size());

    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = verts.size() * sizeof(Vertex);
    mSliderVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mSliderVertexBuffer, 0, verts.data(), bd.size);

    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bd.size  = (indices.size() * sizeof(Index) + 3) & ~3ULL;
    mSliderIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mSliderIndexBuffer, 0, indices.data(), bd.size);
}

void Scene::initializeParticles()
{
    std::vector<QuadVertex>   quadVerts;
    std::vector<ParticleData> particles;

    ParticleSystem::buildQuad(quadVerts);
    ParticleSystem::initParticles(particles, MAX_PARTICLES, 0.35f);

    // Quad vertex buffer — identical pattern to plane/slider
    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = quadVerts.size() * sizeof(QuadVertex);
    mParticleQuadBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mParticleQuadBuffer, 0, quadVerts.data(), bd.size);

    // Particle data buffer — same pattern, different usage flag
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = particles.size() * sizeof(ParticleData);
    mParticleDataBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mParticleDataBuffer, 0, particles.data(), bd.size);

    mParticleCount = static_cast<uint32_t>(particles.size());
    mParticleDrawCount = mParticleCount;  // start with all visible

}

void Scene::setSliderPosition(const float x, const float y, const float z)
{
    mSliderPos[0] = x;
    mSliderPos[1] = y;
    mSliderPos[2] = z;
    mParticleDrawCount = static_cast<uint32_t>(mSliderValue * MAX_PARTICLES);
}

void Scene::setSliderValue(float v)
{
    mSliderValue = v;
    mParticleDrawCount = static_cast<uint32_t>(mSliderValue * MAX_PARTICLES);
}

void Scene::initializePlane()
{
    std::cout << "Initialize plane" << std::endl;
    std::vector<PlaneVertex> vertices;
    std::vector<PlaneIndex>  indices;

    Plane::buildPlane(vertices, indices, 0.15f, 0.25f, 32, 32);

    mPlaneIndexCount = static_cast<uint32_t>(indices.size());

    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = vertices.size() * sizeof(PlaneVertex);
    mPlaneVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mPlaneVertexBuffer, 0, vertices.data(), bd.size);

    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bd.size  = (indices.size() * sizeof(PlaneIndex) + 3) & ~3ULL;
    mPlaneIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mPlaneIndexBuffer, 0, indices.data(), bd.size);
}

// void Scene::updateViewMatrix() {
//     float cx = cos(mCameraState.angles.x);
//     float sx = sin(mCameraState.angles.x);
//     float cy = cos(mCameraState.angles.y);
//     float sy = sin(mCameraState.angles.y);
//     vec3 position = vec3(cx * cy, sx * cy, sy) * std::exp(-mCameraState.zoom);
//     m_uniforms.viewMatrix = glm::lookAt(position, vec3(0.0f), vec3(0, 0, 1));
//     wgpuQueueWriteBuffer(
//         mQueue,
//         mUniformBuffer,
//         offsetof(MyUniforms, viewMatrix),
//         &m_uniforms.viewMatrix,
//         sizeof(MyUniforms::viewMatrix)
//     );
// }