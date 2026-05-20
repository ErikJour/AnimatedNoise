// ── Material IDs ──────────────────────────────────────────────────────────────
const MAT_CAVE:             u32 = 0u;
const MAT_SLIDER:           u32 = 1u;
const MAT_PLANE:            u32 = 2u;
const MAT_PARTICLES:        u32 = 3u;
const MAT_FLOOR:            u32 = 4u;
const MAT_SKYLIGHT:         u32 = 5u;

// ── Scene constants ───────────────────────────────────────────────────────────
const FOV_FACTOR:  f32 = 1.5;
const SPINE_MIN_Y: f32 = -0.15;
const SPINE_MAX_Y: f32 =  0.25;

// ── Uniforms ──────────────────────────────────────────────────────────────────
struct Uniforms {
    time:        f32,
    frequency:   f32,
    amplitude:   f32,
    sliderValue: f32,
    lightPos:    vec3f,
    aspectRatio: f32,
    sliderPos:   vec3f,
    materialId:  u32,
    modelMatrix : mat4x4f,
};

@group(0) @binding(0) var<uniform> u: Uniforms;

// ── Vertex I/O ────────────────────────────────────────────────────────────────
struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color:    vec3f,
    @location(2) normal:   vec3f,
};

struct VertexOutput {
    @builtin(position) clipPos:  vec4f,
    @location(0)       color:    vec3f,
    @location(1)       worldPos: vec3f,
    @location(2)       normal:   vec3f,
};


