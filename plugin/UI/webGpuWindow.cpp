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

        return true;
    }

    // Call once you have the native window handle (after the component has a peer).
    // width/height should be the current editor dimensions in physical pixels.
    bool WebGpuWindow::initSurface(void* nativeHandle, const uint32_t width, const uint32_t height)
    {
#if defined(__APPLE__)
        mSurface = createMetalSurface(mInstance, nativeHandle);
#else
        (void)nativeHandle;
#endif
        if (!mSurface) {
            std::cerr << "Failed to create WGPUSurface." << std::endl;
            return false;
        }
        applySurfaceConfig(width, height);
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
    if (mSurface) {
        wgpuSurfaceUnconfigure(mSurface);
        wgpuSurfaceRelease(mSurface);
        mSurface = nullptr;
    }
    if (mAdapter) {
        // Released here only if initSurface() was never called.
        wgpuAdapterRelease(mAdapter);
        mAdapter = nullptr;
    }
    wgpuQueueRelease(mQueue);
    wgpuDeviceRelease(mDevice);
    //Release instance last
    wgpuInstanceRelease(mInstance);
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


