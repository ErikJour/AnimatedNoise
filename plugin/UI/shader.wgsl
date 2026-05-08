//struct Uniforms {
//    time: f32,
//    frequency: f32,
//    amplitude: f32,
//    _pad: f32
//};
//
//struct VertexInput {
//    @location(0) position: vec3f,
//    @location(1) color: vec3f,
//};
//
//struct VertexOutput {
//    @builtin(position) position: vec4f,
//    @location(0) color: vec3f,
//};
//
//@group(0) @binding(0) var<uniform> u: Uniforms;
//
//const PI: f32 = 3.14159265359;
//
//@vertex
//fn vs_main(in: VertexInput) -> VertexOutput {
//    var out: VertexOutput;
//    var position = in.position;
//    let angle1 = u.time;
//    let c1 = cos(angle1);
//    let s1 = sin(angle1);
//    let R1 = transpose(mat4x4f(
//         c1,  s1, 0.0, 0.0,
//        -s1,  c1, 0.0, 0.0,
//        0.0, 0.0, 1.0, 0.0,
//        0.0, 0.0, 0.0, 1.0,
//    ));
//    let angle2 = 3.0 * PI / 4.0;
//    let c2 = cos(angle2);
//    let s2 = sin(angle2);
//    let R2 = transpose(mat4x4f(
//        1.0, 0.0, 0.0, 0.0,
//        0.0,  c2,  s2, 0.0,
//        0.0, -s2,  c2, 0.0,
//        0.0, 0.0, 0.0, 1.0,
//    ));
//    // Scale the object
//    let S = transpose(mat4x4f(
//        0.3,  0.0, 0.0, 0.0,
//        0.0,  0.3, 0.0, 0.0,
//        0.0,  0.0, 0.3, 0.0,
//        0.0,  0.0, 0.0, 1.0,
//    ));
//
//    // Translate the object
//    let T = transpose(mat4x4f(
//        1.0,  0.0, 0.0, 0.5,
//        0.0,  1.0, 0.0, 0.0,
//        0.0,  0.0, 1.0, 0.0,
//        0.0,  0.0, 0.0, 1.0,
//    ));
//    let homogeneous_position = vec4f(position, 1.0);
//    position = (R2 * R1 * T * S * homogeneous_position).xyz;
//    out.position = vec4f(position.x, position.y, position.z * 0.5 + 0.5, 1.0);
//    out.color = in.color;
//    return out;
//}
//
//@fragment
//fn fs_main(in: VertexOutput) -> @location(0) vec4f {
//    return vec4f(in.color, 1.0);
//
//}

struct Uniforms {
    time:        f32,
    frequency:   f32,
    amplitude:   f32,
    sliderValue: f32,   // was _pad0
    lightPos:    vec3f,
    _pad1:       f32,
};

@group(0) @binding(0) var<uniform> u: Uniforms;

struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color:    vec3f,
};

struct VertexOutput {
    @builtin(position) clipPos:  vec4f,
    @location(0)       color:    vec3f,
    @location(1)       worldPos: vec3f,
};

const FOV_FACTOR:  f32 = 1.5;
const SPINE_MIN_Y: f32 = -0.15;
const SPINE_MAX_Y: f32 =  0.25;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    var pos = in.position;

    // Indicator sentinel: orange (r>0.9, g~0.4, b<0.15)
    let isIndicator = in.color.r > 0.9 && in.color.g > 0.35
                   && in.color.g < 0.45 && in.color.b < 0.15;
    if isIndicator {
        let centerY = SPINE_MIN_Y + u.sliderValue * (SPINE_MAX_Y - SPINE_MIN_Y);
        pos.y = pos.y + centerY;
    }

    // Spine sentinel: r in (0.24, 0.35) keeps this from matching dark cave geometry
    let isSpine = in.color.r > 0.24 && in.color.r < 0.35
               && in.color.g < 0.20 && in.color.b < 0.12;

    if isIndicator || isSpine {
        // Screen-space UI: treat vertex position as clip-space directly, render in front
        out.clipPos = vec4f(pos.x, pos.y, 0.0, 1.0);
    } else {
        let depth   = pos.z + 0.3;
        let inv_d   = FOV_FACTOR / depth;
        out.clipPos = vec4f(pos.x * inv_d, pos.y * inv_d, pos.z * 0.5 + 0.5, 1.0);
    }
    out.color    = in.color;
    out.worldPos = pos;
    return out;
}

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {

    // ── Compute normal BEFORE any branching (uniform control flow required) ──
    let dp_dx  = dpdx(in.worldPos);
    let dp_dy  = dpdy(in.worldPos);
    var normal = normalize(cross(dp_dx, dp_dy));
    if dot(normal, u.lightPos - in.worldPos) < 0.0 {
        normal = -normal;
    }

    // ── Slider indicator ─────────────────────────────────────────────────────
    let isIndicator = in.color.r > 0.9 && in.color.g > 0.35
                   && in.color.g < 0.45 && in.color.b < 0.15;
    if isIndicator {
        return vec4f(1.0, 0.55, 0.15, 1.0);
    }

    // ── Slider spine ──────────────────────────────────────────────────────────
    let isSpine = in.color.r < 0.35 && in.color.g < 0.20 && in.color.b < 0.12;
    if isSpine {
        let fillY  = SPINE_MIN_Y + u.sliderValue * (SPINE_MAX_Y - SPINE_MIN_Y);
        let col    = select(
            vec3f(0.18, 0.09, 0.04),
            vec3f(0.90, 0.45, 0.12) * 0.8,
            in.worldPos.y < fillY
        );
        return vec4f(col, 1.0);
    }

    // ── Cave lighting ─────────────────────────────────────────────────────────
    let toLight     = u.lightPos - in.worldPos;
    let dist        = length(toLight);
    let lightDir    = toLight / dist;
    let attenuation = 1.0 / (1.0 + 4.0 * dist * dist);
    let diffuse     = max(dot(normal, lightDir), 0.0) * attenuation;

    let lampColor = vec3f(1.0, 0.85, 0.55);
    let ambient   = vec3f(0.06, 0.05, 0.10);
    return vec4f(in.color * (ambient + diffuse * lampColor), 1.0);
}