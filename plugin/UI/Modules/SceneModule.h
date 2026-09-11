//
// Created by Erik Jourgensen on 9/9/26.
//

#ifndef ANIMATEDEAST_SCENEMODULE_H
#define ANIMATEDEAST_SCENEMODULE_H

#include <webgpu/webgpu.h>
#include "MeshRenderer.h"
#include "MyUniforms.h"

//==========================================================================================
//Common ground for the per-module scene objects, mirroring the way ThreeDSynth
//holds one object per DSP module. Each UI module owns its geometry, its slice
//of the uniform block, and its own hit testing.
//
//The contract each module follows is
//
//  build()                     geometry and GPU buffers, once, at init
//  render(const MeshRenderer&) record this module's draws
//  writeUniforms(MyUniforms&)  fill this module's fields in the uniform block
//  release()                   drop the GPU buffers
//
//and a module implements only the ones it needs. Only release() is virtual,
//because teardown is the one place Scene walks the modules as a list. Build
//order, draw order and picking are all module-specific, so Scene names each
//module explicitly for those and no dispatch is involved in a frame.
//==========================================================================================
class SceneModule
{
    public:
        virtual ~SceneModule() = default;

        void attach(const WGPUDevice device, const WGPUQueue queue)
        {
            mDevice = device;
            mQueue  = queue;
        }

        virtual void release() = 0;

    protected:
        WGPUDevice mDevice = nullptr;
        WGPUQueue  mQueue  = nullptr;
};

#endif //ANIMATEDEAST_SCENEMODULE_H
