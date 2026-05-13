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

void WebGpuWindow::configurePipeline()
{
    mFragmentState = mScene.getFragmentState();
    mColorTarget = mScene.getColorTarget();
    mBlendState = mScene.getBlendState();

    mPipelineDesc.nextInChain                        = nullptr;
    mPipelineDesc.layout                             = nullptr;
    mPipelineDesc.vertex.bufferCount                 = 0;
    mPipelineDesc.vertex.buffers                     = nullptr;
    // mPipelineDesc.vertex.module                      = mShaderModule;
    mPipelineDesc.vertex.entryPoint                  = WGPU_STR("vs_main");
    mPipelineDesc.vertex.constantCount               = 0;
    mPipelineDesc.vertex.constants                   = nullptr;
    mPipelineDesc.primitive.topology                 = WGPUPrimitiveTopology_TriangleList;
    mPipelineDesc.primitive.stripIndexFormat         = WGPUIndexFormat_Undefined;
    mPipelineDesc.primitive.frontFace                = WGPUFrontFace_CCW;
    mPipelineDesc.primitive.cullMode                 = WGPUCullMode_None;
    mPipelineDesc.fragment                           = &mFragmentState;
    setDefault(depthStencilState);
    depthStencilState.format                         = WGPUTextureFormat_Depth24Plus;
    depthStencilState.depthCompare                   = WGPUCompareFunction_Less;
    depthStencilState.depthWriteEnabled              = WGPUOptionalBool_True;
    depthStencilState.stencilReadMask                = 0;
    depthStencilState.stencilWriteMask               = 0;
    mPipelineDesc.depthStencil                       = &depthStencilState;
    mColorTarget.blend                               = &mBlendState;
    mColorTarget.writeMask                           = WGPUColorWriteMask_All;
    mBlendState.color.srcFactor                      = WGPUBlendFactor_SrcAlpha;
    mBlendState.color.dstFactor                      = WGPUBlendFactor_OneMinusSrcAlpha;
    mBlendState.color.operation                      = WGPUBlendOperation_Add;
    mBlendState.alpha.srcFactor                      = WGPUBlendFactor_Zero;
    mBlendState.alpha.dstFactor                      = WGPUBlendFactor_One;
    mBlendState.alpha.operation                      = WGPUBlendOperation_Add;
    mPipelineDesc.multisample.count                  = 1;
    mPipelineDesc.multisample.mask                   = ~0u;
    mPipelineDesc.multisample.alphaToCoverageEnabled = false;
    mScene.setPipelineDesc(mPipelineDesc);
    // mScene.setShaderModule(mShaderModule);
}

bool WebGpuWindow::initialize()
{
    setWindowColor();
    if (!createInstance())      return false;
    if (!createAdapter())       return false;
    if (!createDevice())        return false;
    if (!createQueue())         return false;
    mScene.init(mDevice, mQueue);
    if (!mScene.createShader())  return false;
    configurePipeline();
    mScene.ConfigureVertexLayout();
    mScene.initializeScene();
    return true;
}

bool WebGpuWindow::initSurface(void* nativeHandle, const uint32_t width, const uint32_t height)
{
    std::cout << "initSurface called: " << width << "x" << height << std::endl;
    mSurface = createMetalSurface(mInstance, nativeHandle);
    if (!mSurface) { std::cerr << "Surface creation failed" << std::endl; return false; }

    applySurfaceConfig(width, height);
    mScene.setSurface(mSurface);
    mScene.setSurfaceFormat(mSurfaceFormat);
    mScene.setSurfaceSize(width, height);
    bool result = mScene.createPipeline();
    std::cout << "createPipeline: " << result << std::endl;
    return result;
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
    mScene.terminate();
    if (mPipelineDesc.layout) { wgpuPipelineLayoutRelease(mPipelineDesc.layout); mPipelineDesc.layout = nullptr; }
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
