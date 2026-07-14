// ── Material IDs ──────────────────────────────────────────────────────────────
const MAT_TEXT:                    u32 = 0u;
const MAT_MASTER_GAIN_SLIDER:      u32 = 1u;
const MAT_COMB_AMT_SLIDER:         u32 = 2u;
const MAT_PLANE:                   u32 = 3u;
const MAT_PARTICLES:               u32 = 4u;
const MAT_FLOOR:                   u32 = 5u;
const MAT_SKYLIGHT:                u32 = 6u;
const MAT_LPG_REZ_SLIDER:          u32 = 7u;
const MAT_NOIS_DENS_SLIDER:        u32 = 8u;
const MAT_LOGO:                    u32 = 9u;

// ── Scene constants ───────────────────────────────────────────────────────────
const FOV_FACTOR:  f32 = 1.5;
const SPINE_MIN_Y: f32 = -0.15;
const SPINE_MAX_Y: f32 =  0.25;

// ── Uniforms ──────────────────────────────────────────────────────────────────
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
    sliderPosition: vec3f
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

fn shadeSpineTube(in: VertexOutput) -> vec4f {
    let cream      = vec3f(0.92, 0.86, 0.72);   // filled / below value
    let grey       = vec3f(0.28, 0.27, 0.25);   // empty  / above value
    let v          = in.color.y;
    let isIndicator = in.color.z > 0.5;

    if (isIndicator) {
        let halfH = 0.048;
        let indicatorCenter = clamp(u.sliderValue, halfH, 1.0 - halfH);
        let dCenter = abs(v - indicatorCenter);
//        if (dCenter > halfH) { discard; }

        let baseAlpha = smoothstep(halfH, 0.0, dCenter);
        var alpha     = baseAlpha * 2.95;
//        if (alpha < 0.01) { discard; }

        let rim = 1.0 - abs(in.normal.y);

        let pulse         = sin(u.time * 4.0) * 0.15 + 0.85;
        let restColor     = vec3f(1.0, 0.38, 0.06);
        let pressedColor  = vec3f(1.0, 0.82, 0.50);   // bright warm-white when held
        let indicatorColor = mix(restColor, pressedColor, u.pressed) * pulse
                           * (0.85 + 0.15 * rim * rim);
        let pressedAlpha  = mix(alpha, min(alpha * 1.25, 1.0), u.pressed);
        return vec4f(indicatorColor, pressedAlpha);
    }

    // ── Spine tube branch ─────────────────────────────────────────────────────
    let pulse     = sin(u.time * 2.0 + v * 6.0) * 0.015 + 0.85;

    let fillEdge = smoothstep(u.sliderValue - 0.03, u.sliderValue + 0.01, v);

    let pulseMask = 1.0 - step(u.sliderValue, v);
    let animPulse = mix(1.0, pulse, pulseMask);

    let creamWithPulse = cream * animPulse * 0.90;
    var finalColor     = mix(creamWithPulse, grey, fillEdge);

    let rim = 1.0 - abs(in.normal.y);
    finalColor += cream * (rim * rim) * 0.18 * (1.0 - fillEdge);

    let meniscusDist = u.sliderValue - v;
    let meniscus     = smoothstep(0.025, 0.0, meniscusDist) * step(0.0, meniscusDist);
    finalColor      += cream * meniscus * 0.60;

    let alpha = mix(0.90, 0.35, fillEdge);

    return vec4f(finalColor, alpha);
}


