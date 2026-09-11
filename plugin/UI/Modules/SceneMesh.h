//
// Created by Erik Jourgensen on 9/9/26.
//

#ifndef ANIMATEDEAST_SCENEMESH_H
#define ANIMATEDEAST_SCENEMESH_H

#include <cstdint>
#include <vector>
#include <webgpu/webgpu.h>

//==========================================================================================
//One uploaded mesh: a vertex buffer, a 16-bit index buffer, and the material
//global/fs_main.wgsl switches on.
//
//Every module used to repeat the same twelve lines of buffer descriptor to get
//here. upload() is that block, once.
//==========================================================================================
struct GpuMesh
{
    WGPUBuffer vertexBuffer = nullptr;
    WGPUBuffer indexBuffer  = nullptr;
    uint32_t   indexCount   = 0;
    uint32_t   materialId   = 0;

    //Index is uint16_t throughout geometries/, so an odd count leaves the buffer
    //size off a 4-byte boundary. This pads the vector rather than rounding the
    //size up, which is the safer of the two: the size is also the byte count read
    //from indices.data(), and rounding it up reads two bytes past the vector.
    template <typename Vertex, typename Index>
    void upload(const WGPUDevice device, const WGPUQueue queue,
                const std::vector<Vertex>& vertices,
                const std::vector<Index>&  indices)
    {
        static_assert(sizeof(Index) == 2, "index buffers are 16-bit");

        release();

        indexCount = static_cast<uint32_t>(indices.size());
        if (vertices.empty() || indices.empty())
            return;

        WGPUBufferDescriptor bd{};
        bd.usage     = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
        bd.size      = vertices.size() * sizeof(Vertex);
        vertexBuffer = wgpuDeviceCreateBuffer(device, &bd);
        wgpuQueueWriteBuffer(queue, vertexBuffer, 0, vertices.data(), bd.size);

        std::vector<Index> padded = indices;
        if (padded.size() % 2 != 0) padded.push_back(0);

        bd.usage    = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
        bd.size     = padded.size() * sizeof(Index);
        indexBuffer = wgpuDeviceCreateBuffer(device, &bd);
        wgpuQueueWriteBuffer(queue, indexBuffer, 0, padded.data(), bd.size);
    }

    void release()
    {
        if (vertexBuffer) { wgpuBufferRelease(vertexBuffer); vertexBuffer = nullptr; }
        if (indexBuffer)  { wgpuBufferRelease(indexBuffer);  indexBuffer  = nullptr; }
        indexCount = 0;
    }
};

#endif //ANIMATEDEAST_SCENEMESH_H
