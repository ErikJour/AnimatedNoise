//
// Created by Erik Jourgensen on 9/9/26.
//

#ifndef ANIMATEDEAST_MESHRENDERER_H
#define ANIMATEDEAST_MESHRENDERER_H

#include <algorithm>
#include <webgpu/webgpu.h>
#include "SceneMesh.h"

//==========================================================================================
//What a module needs in order to draw, and nothing more. Scene owns the bind
//group and the per-material uniform stride; a module only ever says "draw this
//mesh with its material".
//==========================================================================================
class MeshRenderer
{
    public:
        MeshRenderer(const WGPURenderPassEncoder pass, const WGPUBindGroup bindGroup,
                     const uint32_t uniformStride)
            : mPass(pass), mBindGroup(bindGroup), mUniformStride(uniformStride) {}

        void draw(const GpuMesh& mesh) const { draw(mesh, mesh.materialId); }

        //Two meshes share MAT_OSC with the oscillators rather than carrying a
        //material of their own, so the id stays overridable.
        void draw(const GpuMesh& mesh, const uint32_t materialId) const
        {
            drawIndexed(mesh, materialId, mesh.indexCount);
        }

        //THREE's geometry.setDrawRange: draw only the first indexCount indices.
        //The noise cloud varies its particle count this way rather than
        //rebuilding its buffers, which is what generateNoiseParticles() costs
        //in the WebUI.
        void drawRange(const GpuMesh& mesh, const uint32_t indexCount) const
        {
            drawIndexed(mesh, mesh.materialId, std::min(indexCount, mesh.indexCount));
        }

    private:
        void drawIndexed(const GpuMesh& mesh, const uint32_t materialId,
                         const uint32_t indexCount) const
        {
            if (!mesh.vertexBuffer || !mesh.indexBuffer || indexCount == 0)
                return;

            const uint32_t offset = materialId * mUniformStride;
            wgpuRenderPassEncoderSetBindGroup(mPass, 0, mBindGroup, 1, &offset);
            wgpuRenderPassEncoderSetVertexBuffer(mPass, 0, mesh.vertexBuffer, 0,
                                                 wgpuBufferGetSize(mesh.vertexBuffer));
            wgpuRenderPassEncoderSetIndexBuffer(mPass, mesh.indexBuffer, WGPUIndexFormat_Uint16, 0,
                                                wgpuBufferGetSize(mesh.indexBuffer));
            wgpuRenderPassEncoderDrawIndexed(mPass, indexCount, 1, 0, 0, 0);
        }

        WGPURenderPassEncoder mPass;
        WGPUBindGroup         mBindGroup;
        uint32_t              mUniformStride;
};

#endif //ANIMATEDEAST_MESHRENDERER_H
