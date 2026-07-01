//
// Created by Erik Jourgensen on 6/15/26.
//

#include "AnimatedLogo.h"
#include <iostream>
#include <vector>
#include <juce_graphics/juce_graphics.h>
#include <BinaryData.h>

#define WGPU_STR(s) WGPUStringView{s, sizeof(s) - 1}

void AnimatedLogo::init(WGPUDevice device, WGPUQueue queue)
{
    mDevice = device;
    mQueue  = queue;
}

bool AnimatedLogo::initialize(WGPUTextureFormat surfaceFormat, const WGPUDepthStencilState& depthStencilTemplate)
{
    if (!mTexture)
    {
        juce::MemoryInputStream stream(BinaryData::ERIKJOURGENSEN_ICON_B_SMALL_png,
                                       static_cast<size_t>(BinaryData::ERIKJOURGENSEN_ICON_B_SMALL_pngSize),
                                       false);
        auto image = juce::PNGImageFormat().decodeImage(stream);
        if (!image.isValid()) { std::cerr << "Logo PNG decode failed." << std::endl; return false; }

        constexpr int kTexSize = 256;
        auto scaled = image.rescaled(kTexSize, kTexSize, juce::Graphics::highResamplingQuality);

        std::vector<uint8_t> pixels(static_cast<size_t>(kTexSize * kTexSize));
        {
            juce::Image::BitmapData bd(scaled, juce::Image::BitmapData::readOnly);
            for (int y = 0; y < kTexSize; ++y)
                for (int x = 0; x < kTexSize; ++x)
                    pixels[static_cast<size_t>(y * kTexSize + x)] = bd.getPixelColour(x, y).getRed();
        }

        WGPUTextureDescriptor texDesc{};
        texDesc.usage         = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;
        texDesc.dimension     = WGPUTextureDimension_2D;
        texDesc.size          = { static_cast<uint32_t>(kTexSize), static_cast<uint32_t>(kTexSize), 1 };
        texDesc.format        = WGPUTextureFormat_R8Unorm;
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount   = 1;
        mTexture = wgpuDeviceCreateTexture(mDevice, &texDesc);

        WGPUImageCopyTexture dst{};
        dst.texture = mTexture;
        dst.aspect  = WGPUTextureAspect_All;
        WGPUTextureDataLayout layout{};
        layout.bytesPerRow  = static_cast<uint32_t>(kTexSize);
        layout.rowsPerImage = static_cast<uint32_t>(kTexSize);
        const WGPUExtent3D extent{ static_cast<uint32_t>(kTexSize), static_cast<uint32_t>(kTexSize), 1 };
        wgpuQueueWriteTexture(mQueue, &dst, pixels.data(), pixels.size(), &layout, &extent);

        WGPUTextureViewDescriptor tvDesc{};
        tvDesc.format          = WGPUTextureFormat_R8Unorm;
        tvDesc.dimension       = WGPUTextureViewDimension_2D;
        tvDesc.mipLevelCount   = 1;
        tvDesc.arrayLayerCount = 1;
        tvDesc.aspect          = WGPUTextureAspect_All;
        mTextureView = wgpuTextureCreateView(mTexture, &tvDesc);

        WGPUSamplerDescriptor sampDesc{};
        sampDesc.magFilter      = WGPUFilterMode_Linear;
        sampDesc.minFilter      = WGPUFilterMode_Linear;
        sampDesc.mipmapFilter   = WGPUMipmapFilterMode_Linear;
        sampDesc.addressModeU   = WGPUAddressMode_ClampToEdge;
        sampDesc.addressModeV   = WGPUAddressMode_ClampToEdge;
        sampDesc.maxAnisotropy  = 1;
        mSampler = wgpuDeviceCreateSampler(mDevice, &sampDesc);

        struct LogoVertex { float x, y, u, v; };
        constexpr float kRight = -0.865f;
        constexpr float kTop   =  0.97f;
        constexpr float kH     =  0.23f;
        constexpr float kW     = kH * (9.0f / 16.0f);
        const LogoVertex verts[4] = {
            { kRight - kW, kTop - kH,  0.0f, 1.0f },
            { kRight,      kTop - kH,  1.0f, 1.0f },
            { kRight,      kTop,       1.0f, 0.0f },
            { kRight - kW, kTop,       0.0f, 0.0f },
        };
        const uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };

        WGPUBufferDescriptor bd{};
        bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
        bd.size  = sizeof(verts);
        mVertexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
        wgpuQueueWriteBuffer(mQueue, mVertexBuffer, 0, verts, sizeof(verts));

        bd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
        bd.size  = sizeof(indices);
        mIndexBuffer = wgpuDeviceCreateBuffer(mDevice, &bd);
        wgpuQueueWriteBuffer(mQueue, mIndexBuffer, 0, indices, sizeof(indices));
        mIndexCount = 6;
    }

    // Pipeline and bind group — rebuild whenever surface format changes
    if (mBindGroup)    { wgpuBindGroupRelease(mBindGroup);          mBindGroup    = nullptr; }
    if (mPipeline)     { wgpuRenderPipelineRelease(mPipeline);      mPipeline     = nullptr; }
    if (mShaderModule) { wgpuShaderModuleRelease(mShaderModule);    mShaderModule = nullptr; }

    static constexpr const char* kLogoWGSL = R"(
@group(0) @binding(0) var logoTex: texture_2d<f32>;
@group(0) @binding(1) var logoSmp: sampler;
struct LOut {
    @builtin(position) pos: vec4f,
    @location(0) uv: vec2f,
};
@vertex fn vs_logo(@location(0) pos: vec2f, @location(1) uv: vec2f) -> LOut {
    var o: LOut;
    o.pos = vec4f(pos, 0.001, 1.0);
    o.uv  = uv;
    return o;
}
@fragment fn fs_logo(in: LOut) -> @location(0) vec4f {
    let lum   = textureSample(logoTex, logoSmp, in.uv).r;
    let alpha = smoothstep(0.15, 0.65, lum) * 0.75;
    return vec4f(1.0, 1.0, 1.0, alpha);
}
)";
    WGPUShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgslDesc.code        = { kLogoWGSL, strlen(kLogoWGSL) };
    WGPUShaderModuleDescriptor smDesc{};
    smDesc.nextInChain   = &wgslDesc.chain;
    mShaderModule        = wgpuDeviceCreateShaderModule(mDevice, &smDesc);

    WGPUBindGroupLayoutEntry bglEntries[2] = {};
    bglEntries[0].binding               = 0;
    bglEntries[0].visibility            = WGPUShaderStage_Fragment;
    bglEntries[0].texture.sampleType    = WGPUTextureSampleType_Float;
    bglEntries[0].texture.viewDimension = WGPUTextureViewDimension_2D;
    bglEntries[1].binding               = 1;
    bglEntries[1].visibility            = WGPUShaderStage_Fragment;
    bglEntries[1].sampler.type          = WGPUSamplerBindingType_Filtering;

    WGPUBindGroupLayoutDescriptor bglDesc{};
    bglDesc.entryCount = 2;
    bglDesc.entries    = bglEntries;
    WGPUBindGroupLayout bgl = wgpuDeviceCreateBindGroupLayout(mDevice, &bglDesc);

    WGPUPipelineLayoutDescriptor plDesc{};
    plDesc.bindGroupLayoutCount = 1;
    plDesc.bindGroupLayouts     = &bgl;
    WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(mDevice, &plDesc);

    WGPUVertexAttribute attrs[2] = {};
    attrs[0].shaderLocation = 0;
    attrs[0].format         = WGPUVertexFormat_Float32x2;
    attrs[0].offset         = 0;
    attrs[1].shaderLocation = 1;
    attrs[1].format         = WGPUVertexFormat_Float32x2;
    attrs[1].offset         = 2 * sizeof(float);

    WGPUVertexBufferLayout vbl{};
    vbl.attributeCount = 2;
    vbl.attributes     = attrs;
    vbl.arrayStride    = 4 * sizeof(float);
    vbl.stepMode       = WGPUVertexStepMode_Vertex;

    WGPUBlendState blend{};
    blend.color.operation = WGPUBlendOperation_Add;
    blend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blend.alpha.operation = WGPUBlendOperation_Add;
    blend.alpha.srcFactor = WGPUBlendFactor_Zero;
    blend.alpha.dstFactor = WGPUBlendFactor_One;

    WGPUColorTargetState colorTarget{};
    colorTarget.format    = surfaceFormat;
    colorTarget.blend     = &blend;
    colorTarget.writeMask = WGPUColorWriteMask_All;

    WGPUFragmentState frag{};
    frag.module      = mShaderModule;
    frag.entryPoint  = WGPU_STR("fs_logo");
    frag.targetCount = 1;
    frag.targets     = &colorTarget;

    WGPUDepthStencilState depthState = depthStencilTemplate;
    depthState.depthWriteEnabled = WGPUOptionalBool_False;
    depthState.depthCompare      = WGPUCompareFunction_Always;

    WGPURenderPipelineDescriptor pDesc{};
    pDesc.layout                     = pipelineLayout;
    pDesc.vertex.module              = mShaderModule;
    pDesc.vertex.entryPoint          = WGPU_STR("vs_logo");
    pDesc.vertex.bufferCount         = 1;
    pDesc.vertex.buffers             = &vbl;
    pDesc.primitive.topology         = WGPUPrimitiveTopology_TriangleList;
    pDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
    pDesc.primitive.frontFace        = WGPUFrontFace_CCW;
    pDesc.primitive.cullMode         = WGPUCullMode_None;
    pDesc.depthStencil               = &depthState;
    pDesc.multisample.count          = 1;
    pDesc.multisample.mask           = ~0u;
    pDesc.fragment                   = &frag;
    mPipeline = wgpuDeviceCreateRenderPipeline(mDevice, &pDesc);

    wgpuPipelineLayoutRelease(pipelineLayout);

    WGPUBindGroupEntry bgEntries[2] = {};
    bgEntries[0].binding     = 0;
    bgEntries[0].textureView = mTextureView;
    bgEntries[1].binding     = 1;
    bgEntries[1].sampler     = mSampler;

    WGPUBindGroupDescriptor bgDesc{};
    bgDesc.layout     = bgl;
    bgDesc.entryCount = 2;
    bgDesc.entries    = bgEntries;
    mBindGroup = wgpuDeviceCreateBindGroup(mDevice, &bgDesc);

    wgpuBindGroupLayoutRelease(bgl);

    if (!mPipeline || !mBindGroup)
    {
        std::cerr << "Failed to create logo pipeline." << std::endl;
        return false;
    }
    return true;
}

void AnimatedLogo::terminate()
{
    if (mBindGroup)    { wgpuBindGroupRelease(mBindGroup);                                    mBindGroup    = nullptr; }
    if (mPipeline)     { wgpuRenderPipelineRelease(mPipeline);                                mPipeline     = nullptr; }
    if (mIndexBuffer)  { wgpuBufferRelease(mIndexBuffer);                                     mIndexBuffer  = nullptr; }
    if (mVertexBuffer) { wgpuBufferRelease(mVertexBuffer);                                    mVertexBuffer = nullptr; }
    if (mSampler)      { wgpuSamplerRelease(mSampler);                                        mSampler      = nullptr; }
    if (mTextureView)  { wgpuTextureViewRelease(mTextureView);                                mTextureView  = nullptr; }
    if (mTexture)      { wgpuTextureDestroy(mTexture); wgpuTextureRelease(mTexture);          mTexture      = nullptr; }
    if (mShaderModule) { wgpuShaderModuleRelease(mShaderModule);                              mShaderModule = nullptr; }
}

void AnimatedLogo::render(WGPURenderPassEncoder renderPass) const
{
    if (!mPipeline || !mBindGroup || !mVertexBuffer || !mIndexBuffer)
        return;

    wgpuRenderPassEncoderSetPipeline(renderPass, mPipeline);
    wgpuRenderPassEncoderSetBindGroup(renderPass, 0, mBindGroup, 0, nullptr);
    wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, mVertexBuffer, 0, wgpuBufferGetSize(mVertexBuffer));
    wgpuRenderPassEncoderSetIndexBuffer(renderPass, mIndexBuffer, WGPUIndexFormat_Uint16, 0, wgpuBufferGetSize(mIndexBuffer));
    wgpuRenderPassEncoderDrawIndexed(renderPass, mIndexCount, 1, 0, 0, 0);
}
