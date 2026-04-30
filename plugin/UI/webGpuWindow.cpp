//
// Created by Erik Jourgensen on 4/29/26.
//

#include "webGpuWindow.h"

WebGpuWindow::WebGpuWindow() = default;
WebGpuWindow::~WebGpuWindow() = default;

bool WebGpuWindow::initialize()
    {
        //=================================================================
        // Instance
        //=================================================================
        descriptor.nextInChain = nullptr; //This is a way to hook up future wegpu features. Typically set this to null

        WGPUDawnTogglesDescriptor toggles;
        toggles.chain.next = nullptr;
        toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
        toggles.disabledToggleCount = 0;
        toggles.enabledToggleCount = 1;
        const auto toggleName = "enable_immediate_error_handling";
        toggles.enabledToggles = &toggleName;
        descriptor.nextInChain = &toggles.chain;

        mInstance = wgpuCreateInstance(&descriptor); //instance created here. Takes descriptor as argument

        //Quick check to make sure our instance exists
        if (!mInstance) {
            std::cerr << "Failed to create WGPUInstance." << std::endl;
            return false;
        }

        std::cout << "WGPU instance: " << mInstance << std::endl;


        //=================================================================
        // Adapter
        //=================================================================
        adapterOpts.nextInChain = nullptr;
        mAdapter = requestAdapterSync(mInstance, &adapterOpts); //In utilityHelper.h
        if (!mAdapter) {
            std::cerr << "Failed to get WGPUAdapter." << std::endl;
            return false;
        }
        getAdapter(mAdapter, initProperties);
        //Check hardware limits here:
        getLimits(mAdapter, supportedLimits);

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
        shaderDesc.nextInChain = &mShaderCodeDesc.chain;
        mShaderModule = wgpuDeviceCreateShaderModule(mDevice, &shaderDesc);
        //=================================================================
        // Pipeline Descriptor
        //=================================================================
        pipelineDesc.nextInChain = nullptr;
        pipelineDesc.layout = nullptr;
        //RENDER PIPELINE
        // 1 Describe vertex pipeline state
        pipelineDesc.vertex.bufferCount = 0;
        pipelineDesc.vertex.buffers = nullptr;
        pipelineDesc.vertex.module = mShaderModule;
        pipelineDesc.vertex.entryPoint = WGPU_STR("vs_main");
        pipelineDesc.vertex.constantCount = 0;
        pipelineDesc.vertex.constants = nullptr;
        // 2 Describe primitive pipeline state
        pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
        pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
        pipelineDesc.primitive.cullMode = WGPUCullMode_None;
        //blending stage configuration
        pipelineDesc.fragment = &fragmentState;
        // 4 Describe stencil/depth pipeline state
        pipelineDesc.depthStencil = nullptr;
        colorTarget.format = mSurfaceFormat;
        colorTarget.blend = &blendState;
        colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.
        blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blendState.color.operation = WGPUBlendOperation_Add;
        blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
        blendState.alpha.dstFactor = WGPUBlendFactor_One;
        blendState.alpha.operation = WGPUBlendOperation_Add;
        // 5 Describe multi-sampling state
        pipelineDesc.multisample.count = 1;         // Samples per pixel
        pipelineDesc.multisample.mask = ~0u;         // Default value for the mask, meaning "all bits on"
        pipelineDesc.multisample.alphaToCoverageEnabled = false;         // Default value as well (irrelevant for count = 1 anyways)
        // 6 Describe pipeline layout

        // mPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &pipelineDesc);

        return true;
    }

    // Call once you have the native window handle (after the component has a peer).
    // width/height should be the current editor dimensions in physical pixels.
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
    colorTarget.format = mSurfaceFormat;  // now valid
    colorTarget.blend  = &blendState;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    fragmentState.module      = mShaderModule;
    fragmentState.entryPoint  = WGPU_STR("fs_main");
    fragmentState.targetCount = 1;
    fragmentState.targets     = &colorTarget;
    fragmentState.constants   = nullptr;

    pipelineDesc.fragment = &fragmentState;

    mPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &pipelineDesc);
    if (!mPipeline) {
        std::cerr << "Failed to create render pipeline." << std::endl;
        return false;
    }
    return true;
}

void WebGpuWindow::renderFrame() const
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
        colorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments     = &colorAttachment;
        renderPassDesc.depthStencilAttachment = nullptr;


        const WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderSetPipeline(renderPass, mPipeline);
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
    wgpuAdapterGetInfo(adapter, &initProperties);
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


