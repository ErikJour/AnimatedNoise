//
// Created by Erik Jourgensen on 9/9/26.
//

#ifndef ANIMATEDEAST_AMPENVELOPEMODULE_H
#define ANIMATEDEAST_AMPENVELOPEMODULE_H

#include "SceneModule.h"
#include "SceneMesh.h"

//==========================================================================================
//Amp envelope — WebUI/AmpEnvelope/ampGeometries.js.
//
//The ramp geometry is a bare UV grid; mat_adsr.wgsl builds the tube from the
//ADSR uniforms below. Only the two end caps are real geometry.
//==========================================================================================
class AmpEnvelopeModule : public SceneModule
{
    public:
        //The four amp-envelope sliders, as they read: real percentages, 0..100.
        //Both the display remaps and the normalisation live here because both
        //belong to the envelope — ampEnvelope.js remaps before handing the
        //values to createAdsr.js's setADSR, which normalises them.
        struct Parameters
        {
            float attack  = 0.0f;
            float decay   = 0.0f;
            float sustain = 0.0f;
            float release = 0.0f;
        };

        void build();
        void render(const MeshRenderer& renderer) const;
        void writeUniforms(MyUniforms& uniforms) const;
        void release() override;

        void setParameters(const Parameters& parameters) { mParameters = parameters; }

    private:
        //setADSR(): each segment is floored at a visible minimum and the four
        //are then renormalised, so a zero attack still reads as a corner
        //rather than collapsing the ramp.
        static void normaliseSegments(float& a, float& d, float& s, float& r);

        GpuMesh    mMesh;
        Parameters mParameters;
};

#endif //ANIMATEDEAST_AMPENVELOPEMODULE_H
