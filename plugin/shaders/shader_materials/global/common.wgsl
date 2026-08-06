//===============================================
//Common Shader File
//===============================================

//===============================================
//Constants
//===============================================
const FOV_FACTOR:  f32 = 1.5;
const SPINE_MIN_Y: f32 = -0.15;
const SPINE_MAX_Y: f32 =  0.25;
//===========================================
//Level
//===========================================
const MAT_LEVEL:                   u32 = 0u;
const MAT_SKYLIGHT:                u32 = 1u;
//===========================================
//Components
//===========================================
const MAT_LOGO:                    u32 = 2u;
const MAT_TEXT:                    u32 = 3u;
const MAT_TOOLTIP:                 u32 = 4u;
//===========================================
//Visualizations
//===========================================
const MAT_PARTICLES:               u32 = 5u;
//===========================================
//Sliders
//===========================================
const MAT_NOISE_LEVEL_SLIDER:      u32 = 6u;
const MAT_NOISE_LEVEL_MOD_SLIDER:  u32 = 7u;
const MAT_NOISE_DENS_SLIDER:       u32 = 8u;
const MAT_NOISE_DENS_MOD_SLIDER:   u32 = 9u;
const MAT_ATTACK_SLIDER:           u32 = 10u;
const MAT_DECAY_SLIDER:            u32 = 11u;
const MAT_SUSTAIN_SLIDER:          u32 = 12u;
const MAT_RELEASE_SLIDER:          u32 = 13u;
const MAT_FILTER_ONE_SLIDER:       u32 = 14u;
const MAT_FILTER_TWO_SLIDER:       u32 = 15u;
//============================
//Utilities
//============================
const MAT_LIGHT_HELPER:            u32 = 16u;

//===============================================
//Uniforms
//===============================================
struct Uniforms {
    time:           f32,
    frequency:      f32,
    amplitude:      f32,
    sliderValue:    f32,
    lightPos:       vec3f,
    aspectRatio:    f32,
    sliderLevels:   vec4f,
    sliderGlowPos:  array<vec4f, 4>,
    modelMatrix:    mat4x4f,
    viewProjMatrix: mat4x4f,
    projMatrix:     mat4x4f,
    morph :         f32,
    pressed:        f32,
    materialId:     u32,
    resonate:       f32,
    sliderPosition: vec3f,
    cameraPosition: vec3f
};

//===============================================
//In and Out
//===============================================
@group(0) @binding(0) var<uniform> u: Uniforms;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) normal:   vec3f,
    @location(2) color:    vec3f

};

struct VertexOutput {
    @builtin(position) clipPos:  vec4f,
    @location(0)       color:    vec3f,
    @location(1)       worldPos: vec3f,
    @location(2)       normal:   vec3f
};

//===============================================
//Compute Normal
//===============================================
fn computeNormal(worldPos: vec3f) -> vec3f {
    let dp_dx  = dpdx(worldPos);
    let dp_dy  = dpdy(worldPos);
    var normal = normalize(cross(dp_dx, dp_dy));
    if dot(normal, u.lightPos - worldPos) < 0.0 { normal = -normal; }
    return normal;
}


