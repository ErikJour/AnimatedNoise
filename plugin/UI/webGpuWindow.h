//
// Created by Erik Jourgensen on 4/28/26.
//

#ifndef ANIMATEDNOISE_WEBGPUWINDOW_H
#define ANIMATEDNOISE_WEBGPUWINDOW_H

#include <iostream>
#include <webgpu/webgpu.h>
#include "utilityHelper.h"
#include "GpuSurface.h"

#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

class WebGpuWindow
{
public:
    bool initialize()
    {
        //=================================================================
        // Instance
        //=================================================================
        descriptor.nextInChain = nullptr;

#ifdef WEBGPU_BACKEND_DAWN
        WGPUDawnTogglesDescriptor toggles;
        toggles.chain.next = nullptr;
        toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
        toggles.disabledToggleCount = 0;
        toggles.enabledToggleCount = 1;
        const char* toggleName = "enable_immediate_error_handling";
        toggles.enabledToggles = &toggleName;
        descriptor.nextInChain = &toggles.chain;
#endif

        mInstance = wgpuCreateInstance(&descriptor);
        if (!mInstance) {
            std::cerr << "Failed to create WGPUInstance." << std::endl;
            return false;
        }

        //=================================================================
        // Adapter
        //=================================================================
        adapterOpts.nextInChain = nullptr;
        mAdapter = requestAdapterSync(mInstance, &adapterOpts);
        if (!mAdapter) {
            std::cerr << "Failed to get WGPUAdapter." << std::endl;
            return false;
        }
        getAdapter(mAdapter, initProperties);

        //=================================================================
        // Device
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

        // mAdapter is intentionally kept alive here — needed by initSurface()
        // to query WGPUSurfaceCapabilities. It is released inside applySurfaceConfig().

        //=================================================================
        // Queue
        //=================================================================
        mQueue = wgpuDeviceGetQueue(mDevice);

        return true;
    }

    // Call once you have the native window handle (after the component has a peer).
    // width/height should be the current editor dimensions in physical pixels.
    bool initSurface(void* nativeHandle, uint32_t width, uint32_t height)
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

    // Call from juce::Timer::timerCallback() at 60 Hz. Application::MainLoop() in tutorial
    void renderFrame() const
    {
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
        WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDesc);

        // The view holds its own reference to the texture, so we can release
        // the surface texture's reference now (Dawn-specific; wgpu-native requires
        // this to happen after wgpuSurfacePresent instead).
#ifndef WEBGPU_BACKEND_WGPU
        wgpuTextureRelease(surfaceTexture.texture);
#endif

        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.label = WGPU_STR("Frame encoder");
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(mDevice, &encoderDesc);

        WGPURenderPassColorAttachment colorAttachment = {};
        colorAttachment.view       = targetView;
        colorAttachment.loadOp     = WGPULoadOp_Clear;
        colorAttachment.storeOp    = WGPUStoreOp_Store;
        colorAttachment.clearValue = WGPUColor{ 0.1, 0.1, 0.1, 1.0 };
        colorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments     = &colorAttachment;

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

#ifdef WEBGPU_BACKEND_DAWN
        wgpuDeviceTick(mDevice);
#endif

        static int frame = 0;
        if (++frame % 60 == 0)
            std::cout << "Frame " << frame << std::endl;
    }

    // Call from PluginEditor::resized().
    void onResize(uint32_t width, uint32_t height)
    {
        if (!mSurface) return;
        wgpuSurfaceUnconfigure(mSurface);
        applySurfaceConfig(width, height);
    }

    bool hasSurface() const { return mSurface != nullptr; }

    void terminate()
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
        wgpuInstanceRelease(mInstance);
    }

    //==============================================================================
    // Diagnostic helpers
    //==============================================================================
    static void setFeatures(const WGPUAdapter adapter)
    {
        WGPUSupportedFeatures supported = {};
        wgpuAdapterGetFeatures(adapter, &supported);
        std::cout << "Adapter features:" << std::endl;
        std::cout << std::hex;
        for (size_t i = 0; i < supported.featureCount; ++i)
            std::cout << " - 0x" << supported.features[i] << std::endl;
        std::cout << std::dec;
    }

    void getAdapter(const WGPUAdapter adapter, const WGPUAdapterInfo& properties)
    {
        std::cout << "Got adapter: " << adapter << std::endl;
        wgpuAdapterGetInfo(adapter, &initProperties);
        std::cout << "Adapter name: ";
        std::cout.write(properties.device.data, static_cast<std::streamsize>(properties.device.length));
        std::cout << std::endl;
        std::cout << "Adapter backend: 0x" << std::hex << properties.backendType << std::dec << std::endl;
    }

private:
    // Queries surface caps on first call (using mAdapter), then releases mAdapter.
    // Subsequent calls (from onResize) skip the caps query and just reconfigure.
    void applySurfaceConfig(uint32_t width, uint32_t height)
    {
        if (mSurfaceFormat == WGPUTextureFormat_Undefined) {
            WGPUSurfaceCapabilities caps = {};
            wgpuSurfaceGetCapabilities(mSurface, mAdapter, &caps);
            mSurfaceFormat = caps.formats[0];
            wgpuSurfaceCapabilitiesFreeMembers(caps);
            wgpuAdapterRelease(mAdapter);
            mAdapter = nullptr;
        }

        WGPUSurfaceConfiguration config = {};
        config.device       = mDevice;
        config.format       = mSurfaceFormat;
        config.usage        = WGPUTextureUsage_RenderAttachment;
        config.width        = width;
        config.height       = height;
        config.presentMode  = WGPUPresentMode_Fifo;
        config.alphaMode    = WGPUCompositeAlphaMode_Auto;

        wgpuSurfaceConfigure(mSurface, &config);
    }

    WGPUInstanceDescriptor      descriptor    = {};
    WGPUInstance                mInstance     = nullptr;
    WGPURequestAdapterOptions   adapterOpts   = {};
    WGPUAdapterInfo             initProperties = {};
    WGPUAdapter                 mAdapter      = nullptr;
    WGPUDevice                  mDevice       = nullptr;
    WGPUQueue                   mQueue        = nullptr;
    WGPUSurface                 mSurface      = nullptr;
    WGPUTextureFormat           mSurfaceFormat = WGPUTextureFormat_Undefined;
};

#endif //ANIMATEDNOISE_WEBGPUWINDOW_H
