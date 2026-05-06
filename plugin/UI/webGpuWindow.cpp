//
// Created by Erik Jourgensen on 4/29/26.
//
#include "webGpuWindow.h"

WebGpuWindow::WebGpuWindow()    = default;
WebGpuWindow::~WebGpuWindow()   = default;

void WebGpuWindow::setWindowColor()
{
    mRed    = 0.3;
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

bool WebGpuWindow::createShader()
{
    mShaderCodeDesc.chain.next  = nullptr;
    mShaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    mShaderDesc.nextInChain     = &mShaderCodeDesc.chain;

#ifdef DEBUG
    mShaderPath             = DEBUG_SHADER_PATH;
    mShaderSource           = loadShader(mShaderPath.string());
    mShaderCodeDesc.code = { mShaderSource.c_str(), mShaderSource.size() };
    mLastShaderWriteTime    = std::filesystem::last_write_time(mShaderPath);
#else
    mShaderCodeDesc.code = { shaderSource, strlen(shaderSource) };
#endif
    mShaderModule = wgpuDeviceCreateShaderModule(mDevice, &mShaderDesc);
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
    mPipelineDesc.depthStencil                       = nullptr;
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
    InitializeBuffers();
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
    // 1. Create the Buffer
    WGPUBufferDescriptor bufferDesc = {};
    bufferDesc.size                 = sizeof(MyUniforms);
    bufferDesc.usage                = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
    mUniformBuffer                  = wgpuDeviceCreateBuffer(mDevice, &bufferDesc);
    // 2. Create the Bind Group (This connects the buffer to @binding(0))
    WGPUBindGroupEntry entry        = {};
    entry.binding                   = 0;
    entry.buffer                    = mUniformBuffer;
    entry.offset                    = 0;
    entry.size                      = sizeof(MyUniforms);
    WGPUBindGroupDescriptor bgDesc  = {};
    WGPUBindGroupLayout bgl         = wgpuRenderPipelineGetBindGroupLayout(mPipeline, 0);
    bgDesc.layout                   = bgl;
    bgDesc.entryCount               = 1;
    bgDesc.entries                  = &entry;
    mBindGroup                      = wgpuDeviceCreateBindGroup(mDevice, &bgDesc);
    wgpuBindGroupLayoutRelease(bgl);

    return true;
}

void WebGpuWindow::renderFrame(const float currentTime)
{
    #ifdef DEBUG
        auto writeTime = std::filesystem::last_write_time(mShaderPath);
        if (writeTime != mLastShaderWriteTime) {
            mLastShaderWriteTime = writeTime;
            reloadShader();
            return; // skip this frame, render cleanly next frame
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
        WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDesc);
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
        renderPassDesc.depthStencilAttachment   = nullptr;

        const WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

        wgpuRenderPassEncoderSetPipeline(renderPass, mPipeline);
        wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mPointBuffer, 0, wgpuBufferGetSize(mPointBuffer));
        wgpuRenderPassEncoderSetIndexBuffer(renderPass, mIndexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(mIndexBuffer));
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 0, nullptr);
        wgpuRenderPassEncoderDrawIndexed(renderPass, indexCount, 1, 0, 0, 0);

        // wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0); // 3 vertices for triangle
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

        static int frame = 0;
        if (++frame % 60 == 0)
            std::cout << "Frame " << frame << std::endl;
}

void WebGpuWindow::onResize (uint32_t width, uint32_t height)
{
    if (!mSurface) return;
    wgpuSurfaceUnconfigure(mSurface);
    applySurfaceConfig(width, height);
}

void WebGpuWindow::terminate()
{
    if (mPointBuffer)   { wgpuBufferRelease(mPointBuffer); mPointBuffer = nullptr; }
    if (mIndexBuffer)   { wgpuBufferRelease(mIndexBuffer); mIndexBuffer = nullptr; }
    if (mBufferOne)     { wgpuBufferRelease(mBufferOne); mBufferOne = nullptr; }
    if (mBufferTwo)     { wgpuBufferRelease(mBufferTwo); mBufferTwo = nullptr; }
    if (mBindGroup)     { wgpuBindGroupRelease(mBindGroup); mBindGroup = nullptr; }
    if (mUniformBuffer) { wgpuBufferRelease(mUniformBuffer); mUniformBuffer = nullptr; }
    if (mPipeline)      { wgpuRenderPipelineRelease(mPipeline); mPipeline = nullptr; }
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
    MyUniforms      uData;
    uData.time      = time;
    uData.frequency = 10.0f;
    uData.amplitude = 0.5f;
    uData._pad      = 0.0f;
    wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &uData, sizeof(MyUniforms));
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
    requiredLimits.limits.maxBufferSize = 6 * 5 * sizeof(float);
    // Maximum stride between 2 consecutive vertices in the vertex buffer
    requiredLimits.limits.maxVertexBufferArrayStride = 5 * sizeof(float);
    requiredLimits.limits.maxInterStageShaderComponents = 3;


    // [...] Other device limits

    return requiredLimits;
}

void WebGpuWindow::BufferTest()
{
    //========================================================
    //Buffer Experimentation
    //========================================================
    // 1 Create a first buffer
    WGPUBufferDescriptor  bufferDescriptor = {};
    bufferDescriptor.nextInChain = nullptr;
    bufferDescriptor.label = WGPU_STR("GPU-side buffer");
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
    bufferDescriptor.size = 16;
    bufferDescriptor.mappedAtCreation = false;
    mBufferOne = wgpuDeviceCreateBuffer(mDevice, &bufferDescriptor);
    // 2 Create a second buffer
    bufferDescriptor.label = WGPU_STR("Output buffer");
    bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    mBufferTwo = wgpuDeviceCreateBuffer(mDevice, &bufferDescriptor);
    //3 Write input data -- Breaking Here
    std::vector<uint8_t> numbers(16);
    for (uint8_t i = 0; i < 16; ++i) numbers[i] = i;
    wgpuQueueWriteBuffer(mQueue, mBufferOne, 0, numbers.data(), numbers.size());
    //4 Encode and submit the buffer to buffer copy
    WGPUCommandEncoder encoder = {};
    encoder = wgpuDeviceCreateCommandEncoder(mDevice, nullptr);
    wgpuCommandEncoderCopyBufferToBuffer(encoder,
                                        mBufferOne,
                                        0,
                                        mBufferTwo,
                                        0,
                                        16);

    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuCommandEncoderRelease(encoder);
    wgpuQueueSubmit(mQueue, 1, &command);
    wgpuCommandBufferRelease(command);

    // Copy Data
    struct MapContext {
            bool ready;
            WGPUBuffer buffer;
    };
    MapContext context = { false, mBufferTwo };

    auto onBuffer2Mapped = [](WGPUMapAsyncStatus status, WGPUStringView /*message*/,
                                   void* pUserData1, void* /*pUserData2*/) {
    MapContext* ctx = static_cast<MapContext*>(pUserData1);
    ctx->ready = true;
    std::cout << "Buffer 2 mapped with status " << status << std::endl;
    if (status != WGPUMapAsyncStatus_Success) return;

    uint8_t* bufferData = (uint8_t*)wgpuBufferGetConstMappedRange(ctx->buffer, 0, 16);
    for (int i = 0; i < 16; ++i) {
        std::cout << "data[" << i << "] = " << (int)bufferData[i] << std::endl;
    }
    wgpuBufferUnmap(ctx->buffer);
    };

    WGPUBufferMapCallbackInfo2 callbackInfo{};
    callbackInfo.nextInChain = nullptr;
    callbackInfo.mode        = WGPUCallbackMode_AllowSpontaneous;callbackInfo.callback    = onBuffer2Mapped;
    callbackInfo.userdata1   = (void*)&context;
    callbackInfo.userdata2   = nullptr;

    wgpuBufferMapAsync2(mBufferTwo, WGPUMapMode_Read, 0, 16, callbackInfo);

    while (!context.ready) {
        wgpuInstanceProcessEvents(mInstance);
    }
    // 6 Release buffers
    wgpuBufferRelease(mBufferOne);  mBufferOne = nullptr;
    wgpuBufferRelease(mBufferTwo);  mBufferTwo = nullptr;
}

void WebGpuWindow::InitializeBuffers()
{
    indexCount = static_cast<uint32_t>(indexData.size());

    //Attribute 0 — Position (location 0)
    mVertexAttribs[0].shaderLocation = 0;
    mVertexAttribs[0].format         = WGPUVertexFormat_Float32x2;
    mVertexAttribs[0].offset         = 0;

    //Attribute 1 — Color (location 1)
    mVertexAttribs[1].shaderLocation = 1;
    mVertexAttribs[1].format         = WGPUVertexFormat_Float32x3;
    mVertexAttribs[1].offset         = 2 * sizeof(float);

    // Vertex Buffer Layout
    mVertexBufferLayouts.resize(1);
    mVertexBufferLayouts[0].attributeCount = 2;
    mVertexBufferLayouts[0].attributes     = mVertexAttribs.data();
    mVertexBufferLayouts[0].arrayStride    = 5 * sizeof(float);
    mVertexBufferLayouts[0].stepMode       = WGPUVertexStepMode_Vertex;

    //Wiring Into the Pipeline Descriptor
    mPipelineDesc.vertex.bufferCount = 1;
    mPipelineDesc.vertex.buffers     = mVertexBufferLayouts.data();

    WGPUBufferDescriptor bufferDesc{};
    bufferDesc.mappedAtCreation = false;
    //Index Chapter
    // 1. Point buffer (interleaved x, y, r, g, b)
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
    bufferDesc.size  = pointData.size() * sizeof(float);
    bufferDesc.label = WGPU_STR("Point Buffer");
    mPointBuffer = wgpuDeviceCreateBuffer(mDevice, &bufferDesc);
    wgpuQueueWriteBuffer(mQueue, mPointBuffer, 0, pointData.data(), bufferDesc.size);
    // 2. Index buffer (reuse bufferDesc, per tutorial)
    bufferDesc.size  = indexData.size() * sizeof(uint16_t);
    bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
    bufferDesc.size  = (bufferDesc.size + 3ULL) & ~3ULL;
    indexData.resize((indexData.size() + 1ULL) & ~1ULL);
    bufferDesc.label = WGPU_STR("Index Buffer");
    mIndexBuffer     = wgpuDeviceCreateBuffer(mDevice, &bufferDesc);
    wgpuQueueWriteBuffer(mQueue, mIndexBuffer, 0, indexData.data(), bufferDesc.size);
}

#ifdef DEBUG
void WebGpuWindow::reloadShader() {
    mShaderSource                    = loadShader(mShaderPath.string());
    mShaderCodeDesc.code          = { mShaderSource.c_str(), mShaderSource.size() };

    const WGPUShaderModule newModule = wgpuDeviceCreateShaderModule(mDevice, &mShaderDesc);
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




