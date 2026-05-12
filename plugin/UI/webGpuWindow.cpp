//
// Created by Erik Jourgensen on 4/29/26.
//
#include "webGpuWindow.h"

WebGpuWindow::WebGpuWindow()    = default;
WebGpuWindow::~WebGpuWindow()   = default;

void WebGpuWindow::setWindowColor()
{
    mRed    = 0.2;
    mGreen  = 0.25;
    mBlue   = 0.2;
}

bool WebGpuWindow::createInstance()
{
    WGPUInstanceDescriptor descriptor = {};
    descriptor.nextInChain            = nullptr;
    WGPUDawnTogglesDescriptor toggles = {};
    toggles.chain.next                = nullptr;
    toggles.chain.sType               = WGPUSType_DawnTogglesDescriptor;
    toggles.disabledToggleCount       = 0;
    toggles.enabledToggleCount        = 1;
    static constexpr auto toggleName  = "enable_immediate_error_handling";
    toggles.enabledToggles            = &toggleName;
    descriptor.nextInChain            = &toggles.chain;

    mInstance                         = wgpuCreateInstance(&descriptor);

    if (!mInstance) {
        std::cerr << "Failed to create WGPUInstance." << std::endl;
        return false;
    }
    std::cout << "WGPU instance: " << mInstance << std::endl;
    return true;
}

bool WebGpuWindow::createAdapter()
{
    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain               = nullptr;
    mAdapter                              = requestAdapterSync(mInstance, &adapterOpts);
    if (!mAdapter) {
        std::cerr << "Failed to get WGPUAdapter." << std::endl;
        return false;
    }
    getAdapter(mAdapter, mInitProperties);
    getLimits(mAdapter, mSupportedLimits);
    setFeatures(mAdapter);
    return true;
}

bool WebGpuWindow::createDevice()
{
    WGPUDeviceDescriptor deviceDesc         = {};
    deviceDesc.label                        = WGPU_STR("My Device");
    deviceDesc.deviceLostCallbackInfo2.mode = WGPUCallbackMode_AllowSpontaneous;
    deviceDesc.requiredLimits               = nullptr;

    deviceDesc.deviceLostCallbackInfo2.callback = [](WGPUDevice const*,
                                                            const WGPUDeviceLostReason reason,
                                                            const WGPUStringView message,
                                                            void*, void*) {
        if (reason == WGPUDeviceLostReason_Destroyed) return;
        std::cerr << "Device lost: reason " << reason;
        if (message.length > 0) std::cerr << " (" << message.data << ")";
        std::cerr << std::endl;
    };

    deviceDesc.uncapturedErrorCallbackInfo2.callback = [](WGPUDevice const*,
                                                        const WGPUErrorType type,
                                                        const WGPUStringView message,
                                                        void*, void*) {
        std::cerr << "Uncaptured device error: type " << type;
        if (message.length > 0) std::cerr << " (" << message.data << ")";
        std::cerr << std::endl;
    };

    mDevice = requestDeviceSync(mInstance, mAdapter, &deviceDesc);

    if (!mDevice) {
        std::cerr << "Failed to get WGPUDevice." << std::endl;
        return false;
    }

    return true;
}

bool WebGpuWindow::createQueue()
{
    mQueue = wgpuDeviceGetQueue(mDevice);
    if (!mQueue) { return false; }
    std::cout << "WGPUQueue " << mQueue << std::endl;
    return true;
}

#ifdef DEBUG
static std::filesystem::file_time_type latestWriteTime(const std::vector<std::filesystem::path>& paths)
{
    std::filesystem::file_time_type latest{};
    for (const auto& p : paths)
        latest = std::max(latest, std::filesystem::last_write_time(p));
    return latest;
}
#endif

bool WebGpuWindow::createShader()
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

void WebGpuWindow::configurePipeline()
{
    mPipelineDesc.nextInChain                        = nullptr;
    mPipelineDesc.layout                             = nullptr;
    // 1 Describe vertex pipeline state
    mPipelineDesc.vertex.bufferCount                 = 0;
    mPipelineDesc.vertex.buffers                     = nullptr;
    mPipelineDesc.vertex.module                      = mShaderModule;
    mPipelineDesc.vertex.entryPoint                  = WGPU_STR("vs_main");
    mPipelineDesc.vertex.constantCount               = 0;
    mPipelineDesc.vertex.constants                   = nullptr;
    // 2 Describe primitive pipeline state
    mPipelineDesc.primitive.topology                 = WGPUPrimitiveTopology_TriangleList;
    mPipelineDesc.primitive.stripIndexFormat         = WGPUIndexFormat_Undefined;
    mPipelineDesc.primitive.frontFace                = WGPUFrontFace_CCW;
    mPipelineDesc.primitive.cullMode                 = WGPUCullMode_None;
    //blending stage configuration
    mPipelineDesc.fragment                           = &mFragmentState;
    // 4 Describe stencil/depth pipeline state
    setDefault(depthStencilState);
    depthStencilState.format            = WGPUTextureFormat_Depth24Plus;
    depthStencilState.depthCompare      = WGPUCompareFunction_Less;
    depthStencilState.depthWriteEnabled = WGPUOptionalBool_True;
    depthStencilState.stencilReadMask   = 0;
    depthStencilState.stencilWriteMask  = 0;
    mPipelineDesc.depthStencil          = &depthStencilState;
    mColorTarget.blend                               = &mBlendState;
    mColorTarget.writeMask                           = WGPUColorWriteMask_All;
    mBlendState.color.srcFactor                      = WGPUBlendFactor_SrcAlpha;
    mBlendState.color.dstFactor                      = WGPUBlendFactor_OneMinusSrcAlpha;
    mBlendState.color.operation                      = WGPUBlendOperation_Add;
    mBlendState.alpha.srcFactor                      = WGPUBlendFactor_Zero;
    mBlendState.alpha.dstFactor                      = WGPUBlendFactor_One;
    mBlendState.alpha.operation                      = WGPUBlendOperation_Add;
    // 5 Describe multi-sampling state
    mPipelineDesc.multisample.count                  = 1;
    mPipelineDesc.multisample.mask                   = ~0u;
    mPipelineDesc.multisample.alphaToCoverageEnabled = false;
}


bool WebGpuWindow::initialize()
{
    setWindowColor();
    if (!createInstance())      return false;
    if (!createAdapter())       return false;
    if (!createDevice())        return false;
    if (!createQueue())         return false;
    if (!createShader())        return false;
    configurePipeline();
    ConfigureVertexLayout();
    // initializePlane();
    InitializeSlider();
    InitializeProceduralCave();
    initializeParticles();
    return true;
}

bool WebGpuWindow::initSurface(void* nativeHandle, const uint32_t width, const uint32_t height)
{
#if defined(__APPLE__)
    mSurface = createMetalSurface(mInstance, nativeHandle);
#endif
    if (!mSurface) return false;

    applySurfaceConfig(width, height);
    return createPipeline();
}

bool WebGpuWindow::createPipeline()
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
    bufferDesc.size                 = 4 * mUniformStride;  // one slot per material
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

    wgpuBindGroupLayoutRelease(bgl);
    if (!createParticlePipeline()) return false;
    return true;
}

void WebGpuWindow::renderFrame(const float currentTime)
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
        // wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 0, nullptr);


    // Cave
    if (mCaveVertexBuffer && mCaveIndexBuffer && mCaveIndexCount > 0)
    {
        const uint32_t offset = MAT_CAVE * mUniformStride;
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 1, &offset);
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mCaveVertexBuffer, 0, wgpuBufferGetSize(mCaveVertexBuffer));
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, mCaveIndexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(mCaveIndexBuffer));
        wgpuRenderPassEncoderDrawIndexed(renderPass, mCaveIndexCount, 1, 0, 0, 0);
    }

    // Slider
    if (mSliderVertexBuffer && mSliderIndexBuffer && mSliderIndexCount > 0)
    {
        const uint32_t offset = MAT_SLIDER * mUniformStride;
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 1, &offset);
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mSliderVertexBuffer, 0, wgpuBufferGetSize(mSliderVertexBuffer));
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, mSliderIndexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(mSliderIndexBuffer));
        wgpuRenderPassEncoderDrawIndexed(renderPass, mSliderIndexCount, 1, 0, 0, 0);
    }

    // Plane
    if (mPlaneVertexBuffer && mPlaneIndexBuffer && mPlaneIndexCount > 0)
    {
        const uint32_t offset = MAT_PLANE * mUniformStride;
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 1, &offset);
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mPlaneVertexBuffer, 0, wgpuBufferGetSize(mPlaneVertexBuffer));
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, mPlaneIndexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(mPlaneIndexBuffer));
        wgpuRenderPassEncoderDrawIndexed(renderPass, mPlaneIndexCount, 1, 0, 0, 0);
    }

    // Particles
    if (mParticleQuadBuffer && mParticleDataBuffer && mParticleCount > 0)
    {
        wgpuRenderPassEncoderSetPipeline(renderPass, mParticlePipeline);  // ← swap pipeline
        const uint32_t offset = MAT_PARTICLES * mUniformStride;
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 1, &offset);
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mParticleQuadBuffer, 0, wgpuBufferGetSize(mParticleQuadBuffer));
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, mParticleDataBuffer, 0, wgpuBufferGetSize(mParticleDataBuffer));
        wgpuRenderPassEncoderDraw(renderPass, 6, mParticleDrawCount, 0, 0);
        wgpuRenderPassEncoderSetPipeline(renderPass, mPipeline);          // ← restore main pipeline
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

void WebGpuWindow::onResize (uint32_t width, uint32_t height)
{
    if (!mSurface) return;
    wgpuSurfaceUnconfigure(mSurface);
    applySurfaceConfig(width, height);
    // updateDepthTexture(width, height);
}

void WebGpuWindow::terminate()
{
    if (mParticleQuadBuffer)  { wgpuBufferRelease(mParticleQuadBuffer);  mParticleQuadBuffer  = nullptr; }
    if (mParticleDataBuffer)  { wgpuBufferRelease(mParticleDataBuffer);  mParticleDataBuffer  = nullptr; }
    if (mDepthTextureView) { wgpuTextureViewRelease(mDepthTextureView); mDepthTextureView = nullptr; }
    if (mDepthTexture)     { wgpuTextureDestroy(mDepthTexture); wgpuTextureRelease(mDepthTexture); mDepthTexture = nullptr; }
    if (mPlaneVertexBuffer) { wgpuBufferRelease(mPlaneVertexBuffer); mPlaneVertexBuffer = nullptr; }
    if (mPlaneIndexBuffer)  { wgpuBufferRelease(mPlaneIndexBuffer);  mPlaneIndexBuffer  = nullptr; }
    if (mCaveVertexBuffer)   { wgpuBufferRelease(mCaveVertexBuffer); mCaveVertexBuffer = nullptr; }
    if (mCaveIndexBuffer)   { wgpuBufferRelease(mCaveIndexBuffer); mCaveIndexBuffer = nullptr; }
    if (mSliderVertexBuffer) { wgpuBufferRelease(mSliderVertexBuffer); mSliderVertexBuffer = nullptr; }
    if (mSliderIndexBuffer)  { wgpuBufferRelease(mSliderIndexBuffer);  mSliderIndexBuffer  = nullptr; }
    if (mBindGroup)     { wgpuBindGroupRelease(mBindGroup); mBindGroup = nullptr; }
    if (mUniformBuffer) { wgpuBufferRelease(mUniformBuffer); mUniformBuffer = nullptr; }
    if (mParticlePipeline) { wgpuRenderPipelineRelease(mParticlePipeline); mParticlePipeline = nullptr; }
    if (mPipeline)      { wgpuRenderPipelineRelease(mPipeline); mPipeline = nullptr; }
    if (mPipelineDesc.layout) { wgpuPipelineLayoutRelease(mPipelineDesc.layout); mPipelineDesc.layout = nullptr; }
    if (mSurface)       { wgpuSurfaceUnconfigure(mSurface); wgpuSurfaceRelease(mSurface); mSurface = nullptr; }
    if (mQueue)         { wgpuQueueRelease(mQueue); mQueue = nullptr; }
    if (mDevice)        { wgpuDeviceRelease(mDevice); mDevice = nullptr; }
    if (mAdapter)       { wgpuAdapterRelease(mAdapter); mAdapter = nullptr; }
    if (mInstance)      { wgpuInstanceRelease(mInstance); mInstance = nullptr; }
}

void WebGpuWindow::setFeatures(const WGPUAdapter adapter)
{
    WGPUSupportedFeatures supported = {};
    wgpuAdapterGetFeatures(adapter, &supported);
    std::cout << "Adapter features:" << std::endl;
    std::cout << std::hex;
    for (size_t i = 0; i < supported.featureCount; ++i)
        std::cout << " - 0x" << supported.features[i] << std::endl;
    std::cout << std::dec;
}

void WebGpuWindow::getAdapter(const WGPUAdapter adapter, const WGPUAdapterInfo& properties)
{
    std::cout << "Got adapter: " << adapter << std::endl;
    wgpuAdapterGetInfo(adapter, &mInitProperties);
    std::cout << "Adapter name: ";
    std::cout.write(properties.device.data, static_cast<std::streamsize>(properties.device.length));
    std::cout << std::endl;
    std::cout << "Adapter backend: 0x" << std::hex << properties.backendType << std::dec << std::endl;
}

void WebGpuWindow::getLimits(WGPUAdapter adapter, WGPUSupportedLimits &limits)
{
    bool success = wgpuAdapterGetLimits(adapter, &limits) == WGPUStatus_Success;
    if (success) {
        std::cout << "Adapter limits:" << std::endl;
        std::cout << " - maxTextureDimension1D: " << limits.limits.maxTextureDimension1D << std::endl;
        std::cout << " - maxTextureDimension2D: " << limits.limits.maxTextureDimension2D << std::endl;
        std::cout << " - maxTextureDimension3D: " << limits.limits.maxTextureDimension3D << std::endl;
        std::cout << " - maxTextureArrayLayers: " << limits.limits.maxTextureArrayLayers << std::endl;
    }
}

void WebGpuWindow::setUniforms(WGPUQueue queue, const WGPUBuffer uniformBuffer, const float time) const
{
    MyUniforms base{};
    base.time        = time;
    base.frequency   = 10.0f;
    base.amplitude   = 0.5f;
    base.sliderValue  = mSliderValue;
    base.lightPos[0]  = 0.25f;
    base.lightPos[1]  = 0.10f;
    base.lightPos[2]  = 0.35f;
    base.sliderPos[0] = mSliderPos[0];
    base.sliderPos[1] = mSliderPos[1];
    base.sliderPos[2] = mSliderPos[2];

    const uint32_t ids[4] = { MAT_CAVE, MAT_SLIDER, MAT_PLANE, MAT_PARTICLES };
    for (uint32_t i = 0; i < 4; ++i) {
        base.materialId = ids[i];
        wgpuQueueWriteBuffer(queue, uniformBuffer, i * mUniformStride, &base, sizeof(MyUniforms));
    }
}

void WebGpuWindow::wgpuPollEvents([[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser)
{
    wgpuDeviceTick(device);
}

void WebGpuWindow::setDefault(WGPULimits &limits) {
    limits.maxTextureDimension1D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension2D = WGPU_LIMIT_U32_UNDEFINED;
    limits.maxTextureDimension3D = WGPU_LIMIT_U32_UNDEFINED;
}

void WebGpuWindow::setDefault(WGPUStencilFaceState& stencilFaceState)
{
    stencilFaceState.compare      = WGPUCompareFunction_Always;
    stencilFaceState.failOp       = WGPUStencilOperation_Keep;
    stencilFaceState.depthFailOp  = WGPUStencilOperation_Keep;
    stencilFaceState.passOp       = WGPUStencilOperation_Keep;
}

void WebGpuWindow::setDefault(WGPUDepthStencilState& depthStencilState)
{
    depthStencilState.format             = WGPUTextureFormat_Undefined;
    depthStencilState.depthWriteEnabled  = WGPUOptionalBool_False;
    depthStencilState.depthCompare       = WGPUCompareFunction_Always;
    depthStencilState.stencilReadMask    = 0xFFFFFFFF;
    depthStencilState.stencilWriteMask   = 0xFFFFFFFF;
    depthStencilState.depthBias          = 0;
    depthStencilState.depthBiasSlopeScale = 0;
    depthStencilState.depthBiasClamp     = 0;
    setDefault(depthStencilState.stencilFront);
    setDefault(depthStencilState.stencilBack);
}

WGPURequiredLimits WebGpuWindow::GetRequiredLimits(WGPUAdapter adapter)
{
    // Get adapter supported limits, in case we need them
    WGPUSupportedLimits supportedLimits;
    supportedLimits.nextInChain = nullptr;
    wgpuAdapterGetLimits(adapter, &supportedLimits);
    WGPURequiredLimits requiredLimits{};
    setDefault(requiredLimits.limits);
    // We use at most 1 vertex attribute for now
    requiredLimits.limits.maxVertexAttributes = 2;
    // We should also tell that we use 1 vertex buffers
    requiredLimits.limits.maxVertexBuffers = 1;
    // Maximum size of a buffer is 6 vertices of 2 float each
    requiredLimits.limits.maxBufferSize = 15 * 5 * sizeof(float);
    // Maximum stride between 2 consecutive vertices in the vertex buffer
    requiredLimits.limits.maxVertexBufferArrayStride = 9 * sizeof(float);
    requiredLimits.limits.maxInterStageShaderComponents = 3;
    return requiredLimits;
}

void WebGpuWindow::ConfigureVertexLayout()
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
    mVertexBufferLayouts[0].arrayStride    = 9 * sizeof(float);   // ← restored
    mVertexBufferLayouts[0].stepMode       = WGPUVertexStepMode_Vertex;

    mPipelineDesc.vertex.bufferCount = 1;                         // ← restored
    mPipelineDesc.vertex.buffers     = mVertexBufferLayouts.data();
}

#ifdef DEBUG
void WebGpuWindow::reloadShader()
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

void WebGpuWindow::InitializeProceduralCave()
{
    std::vector<Vertex> verts;
    std::vector<Index>  indices;
    mPerlinCave.buildCaveGeometry(verts, indices);

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

void WebGpuWindow::InitializeLoadedCave()
{
    std::vector<float>    pointData;
    std::vector<uint16_t> indexData;

    const bool success = ResourceManager::loadGeometry(RESOURCE_DIR "/cave.txt",
                                                        pointData,
                                                        indexData,
                                                        3);
    if (!success) {
        std::cerr << "Could not load geometry!" << std::endl;
        return;
    }

    mCaveIndexCount = static_cast<uint32_t>(indexData.size());

    WGPUBufferDescriptor bd{};
    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bd.size  = pointData.size() * sizeof(float);
    mCaveVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mCaveVertexBuffer, 0, pointData.data(), bd.size);

    bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bd.size  = (indexData.size() * sizeof(uint16_t) + 3) & ~3ULL;
    mCaveIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
    wgpuQueueWriteBuffer(mQueue, mCaveIndexBuffer, 0, indexData.data(), bd.size);
}

void WebGpuWindow::InitializeSlider()
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

void WebGpuWindow::setSliderPosition(const float x, const float y, const float z)
{
    mSliderPos[0] = x;
    mSliderPos[1] = y;
    mSliderPos[2] = z;
    mParticleDrawCount = static_cast<uint32_t>(mSliderValue * MAX_PARTICLES);
}

void WebGpuWindow::setSliderValue(float v)
{
    mSliderValue = v;
    mParticleDrawCount = static_cast<uint32_t>(mSliderValue * MAX_PARTICLES);
}


void WebGpuWindow::initializePlane()
{
    std::cout << "Initialize plane" << std::endl;
    std::vector<PlaneVertex> vertices;
    std::vector<PlaneIndex>  indices;

    mPlane.buildPlane(vertices, indices, 0.15f, 0.25f, 32, 32);

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

void WebGpuWindow::initializeParticles()
{
    std::vector<QuadVertex>   quadVerts;
    std::vector<ParticleData> particles;

    mParticleSystem.buildQuad(quadVerts);
    mParticleSystem.initParticles(particles, MAX_PARTICLES, 0.35f);

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

bool WebGpuWindow::createParticlePipeline()
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
    mParticleFragmentState.module      = mShaderModule;
    mParticleFragmentState.entryPoint  = WGPU_STR("fs_particle");
    mParticleFragmentState.targetCount = 1;
    mParticleFragmentState.targets     = &mColorTarget;   // same color target as main pipeline
    mParticleFragmentState.constants   = nullptr;

    // Reuse the same fragment state — swap entry point if you want a dedicated one
    mParticlePipelineDesc.fragment = &mParticleFragmentState;  // separate state

    mParticlePipeline = wgpuDeviceCreateRenderPipeline(mDevice, &mParticlePipelineDesc);
    if (!mParticlePipeline) {
        std::cerr << "Failed to create particle render pipeline." << std::endl;
        return false;
    }
    return true;
}



