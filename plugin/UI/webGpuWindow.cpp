//
// Created by Erik Jourgensen on 4/29/26.
//

#include "webGpuWindow.h"

WebGpuWindow::WebGpuWindow() = default;
WebGpuWindow::~WebGpuWindow() = default;

void WebGpuWindow::setWindowColor()
{
    mRed = 0.3;
    mGreen = 0.25;
    mBlue = 0.2;
}

bool WebGpuWindow::initialize()
    {
        setWindowColor();
        //=================================================================
        // Instance
        //=================================================================
        mDescriptor.nextInChain = nullptr; //This is a way to hook up future wegpu features. Typically set this to null

        WGPUDawnTogglesDescriptor toggles;
        toggles.chain.next = nullptr;
        toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
        toggles.disabledToggleCount = 0;
        toggles.enabledToggleCount = 1;
        const auto toggleName = "enable_immediate_error_handling";
        toggles.enabledToggles = &toggleName;
        mDescriptor.nextInChain = &toggles.chain;

        mInstance = wgpuCreateInstance(&mDescriptor); //instance created here. Takes descriptor as argument

        //Quick check to make sure our instance exists
        if (!mInstance) {
            std::cerr << "Failed to create WGPUInstance." << std::endl;
            return false;
        }
        std::cout << "WGPU instance: " << mInstance << std::endl;

        //=================================================================
        // Adapter
        //=================================================================
        mAdapterOpts.nextInChain = nullptr;
        mAdapter = requestAdapterSync(mInstance, &mAdapterOpts); //In utilityHelper.h
        if (!mAdapter) {
            std::cerr << "Failed to get WGPUAdapter." << std::endl;
            return false;
        }
        getAdapter(mAdapter, mInitProperties);
        //Check hardware limits here:
        getLimits(mAdapter, mSupportedLimits);

        //set Features
        setFeatures(mAdapter);

        //=================================================================
        // Device - Our main object to interact with
        //=================================================================
        WGPUDeviceDescriptor deviceDesc = {};
        deviceDesc.label = WGPU_STR("My Device");
        deviceDesc.deviceLostCallbackInfo2.callback = [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView message, void*, void*) {
            if (reason == WGPUDeviceLostReason_Destroyed) return;
            std::cerr << "Device lost: reason " << reason;
            if (message.length > 0) std::cerr << " (" << message.data << ")";
            std::cerr << std::endl;
        };
        deviceDesc.deviceLostCallbackInfo2.mode = WGPUCallbackMode_AllowSpontaneous;
        deviceDesc.uncapturedErrorCallbackInfo2.callback = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView message, void*, void*) {
            std::cerr << "Uncaptured device error: type " << type;
            if (message.length > 0) std::cerr << " (" << message.data << ")";
            std::cerr << std::endl;
        };

        mDevice = requestDeviceSync(mInstance, mAdapter, &deviceDesc);
        if (!mDevice) {
            std::cerr << "Failed to get WGPUDevice." << std::endl;
            return false;
        }
        //=================================================================
        // Queue
        //=================================================================
        mQueue = wgpuDeviceGetQueue(mDevice);

        //=================================================================
        // Shader Module
        //=================================================================
        // Set the chained struct's header
        mShaderCodeDesc.chain.next = nullptr;
        mShaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
        mShaderCodeDesc.code        = { shaderSource, strlen(shaderSource) }; // ← missing
        mShaderDesc.nextInChain = &mShaderCodeDesc.chain;
        mShaderModule = wgpuDeviceCreateShaderModule(mDevice, &mShaderDesc);
        //=================================================================
        // Pipeline Descriptor
        //=================================================================
        mPipelineDesc.nextInChain = nullptr;
        mPipelineDesc.layout = nullptr;
        //RENDER PIPELINE
        // 1 Describe vertex pipeline state
        mPipelineDesc.vertex.bufferCount = 0;
        mPipelineDesc.vertex.buffers = nullptr;
        mPipelineDesc.vertex.module = mShaderModule;
        mPipelineDesc.vertex.entryPoint = WGPU_STR("vs_main");
        mPipelineDesc.vertex.constantCount = 0;
        mPipelineDesc.vertex.constants = nullptr;
        // 2 Describe primitive pipeline state
        mPipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        mPipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        mPipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
        mPipelineDesc.primitive.cullMode = WGPUCullMode_None;
        //blending stage configuration
        mPipelineDesc.fragment = &mFragmentState;
        // 4 Describe stencil/depth pipeline state
        mPipelineDesc.depthStencil = nullptr;
        mColorTarget.blend = &mBlendState;
        mColorTarget.writeMask = WGPUColorWriteMask_All;
        mBlendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        mBlendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        mBlendState.color.operation = WGPUBlendOperation_Add;
        mBlendState.alpha.srcFactor = WGPUBlendFactor_Zero;
        mBlendState.alpha.dstFactor = WGPUBlendFactor_One;
        mBlendState.alpha.operation = WGPUBlendOperation_Add;
        // 5 Describe multi-sampling state
        mPipelineDesc.multisample.count = 1;         // Samples per pixel
        mPipelineDesc.multisample.mask = ~0u;         // Default value for the mask, meaning "all bits on"
        mPipelineDesc.multisample.alphaToCoverageEnabled = false;         // Default value as well (irrelevant for count = 1 anyways)
        // 6 Describe pipeline layout
        //========================================================
        //Buffer Experimentation
        //========================================================
        // 1 Create a first buffer
        mBufferDescriptor.nextInChain = nullptr;
        mBufferDescriptor.label = WGPU_STR("GPU-side buffer");
        mBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
        mBufferDescriptor.size = 16;
        mBufferDescriptor.mappedAtCreation = false;
        mBufferOne = wgpuDeviceCreateBuffer(mDevice, &mBufferDescriptor);
        // 2 Create a second buffer
        mBufferDescriptor.label = WGPU_STR("Output buffer");
        mBufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        mBufferTwo = wgpuDeviceCreateBuffer(mDevice, &mBufferDescriptor);

        // // 3 Write input data -- Breaking Here

        // std::vector<uint8_t> numbers(16);
        // for (uint8_t i = 0; i < 16; ++i) numbers[i] = i;
        // wgpuQueueWriteBuffer(mQueue, mBufferOne, 0, numbers.data(), numbers.size());
        //
        // // 4 Encode and submit the buffer to buffer copy
        // mEncoder = wgpuDeviceCreateCommandEncoder(mDevice, nullptr);
        // WGPUCommandBuffer command = wgpuCommandEncoderFinish(mEncoder, nullptr);
        // wgpuCommandEncoderRelease(mEncoder);
        // wgpuQueueSubmit(mQueue, 1, &command);
        // wgpuCommandBufferRelease(command);
        // wgpuCommandEncoderCopyBufferToBuffer(mEncoder, mBufferOne, 0, mBufferTwo, 0, 16);
        // //Copy Data
        // auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void* pUserData) {
        // // We know by convention with ourselves that the user data is a pointer to 'ready':
        // bool* pReady = reinterpret_cast<bool*>(pUserData);
        // // We set ready to 'true'
        // *pReady = true;
        //
        // std::cout << "Buffer 2 mapped with status " << status << std::endl;
        // };
        // wgpuBufferMapAsync(mBufferTwo, WGPUMapMode_Read, 0, 16, onBuffer2Mapped, nullptr /*pUserData*/);
        // 5 Read buffer data back

        // 6 Release buffers

        return true;
    }


bool WebGpuWindow::initSurface(void* nativeHandle, uint32_t width, uint32_t height)
{
#if defined(__APPLE__)
    mSurface = createMetalSurface(mInstance, nativeHandle);
#endif
    if (!mSurface) return false;

    applySurfaceConfig(width, height);  // sets mSurfaceFormat
    return createPipeline();            // now safe to use mSurfaceFormat
}

bool WebGpuWindow::createPipeline()
{

    mColorTarget.format = mSurfaceFormat;  // now valid
    mColorTarget.blend  = &mBlendState;
    mColorTarget.writeMask = WGPUColorWriteMask_All;

    mFragmentState.module      = mShaderModule;
    mFragmentState.entryPoint  = WGPU_STR("fs_main");
    mFragmentState.targetCount = 1;
    mFragmentState.targets     = &mColorTarget;
    mFragmentState.constants   = nullptr;

    mPipelineDesc.fragment = &mFragmentState;

    mPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &mPipelineDesc);
    if (!mPipeline) {
        std::cerr << "Failed to create render pipeline." << std::endl;
        return false;
    }

        //Shader
        // 1. Create the Buffer
        WGPUBufferDescriptor bufferDesc = {};
        bufferDesc.size = sizeof(MyUniforms);
        bufferDesc.usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst;
        mUniformBuffer = wgpuDeviceCreateBuffer(mDevice, &bufferDesc);

        // 2. Create the Bind Group (This connects the buffer to @binding(0))
        // We'll get the layout from the pipeline after it's created
        WGPUBindGroupEntry entry = {};
        entry.binding = 0;
        entry.buffer = mUniformBuffer;
        entry.offset = 0;
        entry.size = sizeof(MyUniforms);

        WGPUBindGroupDescriptor bgDesc = {};
        bgDesc.layout = wgpuRenderPipelineGetBindGroupLayout(mPipeline, 0);
        bgDesc.entryCount = 1;
        bgDesc.entries = &entry;
        mBindGroup = wgpuDeviceCreateBindGroup(mDevice, &bgDesc);

    return true;
}

void WebGpuWindow::renderFrame(const float currentTime) const
{
        if (!mSurface) return; //Surface check

        WGPUSurfaceTexture surfaceTexture = {};
        wgpuSurfaceGetCurrentTexture(mSurface, &surfaceTexture);

        if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) return;

        WGPUTextureViewDescriptor viewDesc = {};
        viewDesc.format = mSurfaceFormat;
        viewDesc.dimension = WGPUTextureViewDimension_2D;
        viewDesc.mipLevelCount = 1;
        viewDesc.arrayLayerCount = 1;
        viewDesc.aspect = WGPUTextureAspect_All;

        //================================================================
        setUniforms(mQueue, mUniformBuffer, currentTime);
        //================================================================


        WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDesc);

        // The view holds its own reference to the texture, so we can release
        // the surface texture's reference now (Dawn-specific; wgpu-native requires
        // this to happen after wgpuSurfacePresent instead).
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
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments     = &colorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;


        const WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderSetPipeline(renderPass, mPipeline);
        wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 0, nullptr);
        wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0); // 3 vertices for your triangle
        wgpuRenderPassEncoderEnd(renderPass);
        wgpuRenderPassEncoderRelease(renderPass);



        WGPUCommandBufferDescriptor cmdDesc = {};
        cmdDesc.label = WGPU_STR("Frame command buffer");
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdDesc);
        wgpuCommandEncoderRelease(encoder);

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
    if (mBufferOne)
    {
        wgpuBufferRelease(mBufferOne);
        mBufferOne = nullptr;
    }
    if (mBufferTwo)
    {
        wgpuBufferRelease(mBufferTwo);
        mBufferOne = nullptr;
    }
    if (mBindGroup)
    {
        wgpuBindGroupRelease(mBindGroup);
        mBindGroup = nullptr;
    }
    if (mUniformBuffer)
    {
        wgpuBufferRelease(mUniformBuffer);
        mUniformBuffer = nullptr;
    }

    if (mPipeline) {
        wgpuRenderPipelineRelease(mPipeline);
        mPipeline = nullptr;
    }

    if (mSurface) {
        wgpuSurfaceUnconfigure(mSurface);
        wgpuSurfaceRelease(mSurface);
        mSurface = nullptr;
    }

    if (mQueue) {
        wgpuQueueRelease(mQueue);
        mQueue = nullptr;
    }

    if (mDevice) {
        wgpuDeviceRelease(mDevice);
        mDevice = nullptr;
    }

    if (mAdapter) {
        wgpuAdapterRelease(mAdapter);
        mAdapter = nullptr;
    }

    if (mInstance) {
        wgpuInstanceRelease(mInstance);
        mInstance = nullptr;
    }
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
    MyUniforms uData;
    uData.time = time;
    // mRed = (std::sin(time) * 0.5f) + 0.5f;
    uData.frequency = 10.0f;
    uData.amplitude = 0.5f;
    uData._pad = 0.0f;
    wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &uData, sizeof(MyUniforms));
}

void WebGpuWindow::wgpuPollEvents([[maybe_unused]] WGPUDevice device, [[maybe_unused]] bool yieldToWebBrowser)
{
    wgpuDeviceTick(device);
}



