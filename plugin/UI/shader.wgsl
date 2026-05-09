struct Uniforms {
    time:        f32,
    frequency:   f32,
    amplitude:   f32,
    sliderValue: f32,
    lightPos:    vec3f,
    _pad1:       f32,
    sliderPos:   vec3f,
    _pad2:       f32,
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

    let isIndicator = in.color.r > 0.9 && in.color.g > 0.35
                   && in.color.g < 0.45 && in.color.b < 0.15;
    if isIndicator {
        let centerY = SPINE_MIN_Y + u.sliderValue * (SPINE_MAX_Y - SPINE_MIN_Y);
        pos.y = pos.y + centerY;
    }

    let isSpine = in.color.r > 0.24 && in.color.r < 0.35
               && in.color.g < 0.20 && in.color.b < 0.12;

    if isIndicator || isSpine {
        pos = pos + u.sliderPos;
        out.clipPos = vec4f(pos.x, pos.y, pos.z * 0.5 + 0.5, 1.0);

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

    // ── Compute normal ──
    let dp_dx  = dpdx(in.worldPos);
    let dp_dy  = dpdy(in.worldPos);
    var normal = normalize(cross(dp_dx, dp_dy));
    if dot(normal, u.lightPos - in.worldPos) < 0.0 {
        normal = -normal;
    }

    // ── Slider indicator (The "Fresco Pink" knob) ────────────────────────────
    // Updated logic to match the new pink sentinel if you change vertex data,
    // or just hard-override the output color here.
    let isIndicator = in.color.r > 0.9 && in.color.g > 0.35
                   && in.color.g < 0.45 && in.color.b < 0.15;
    if isIndicator {
        // A soft, mineral Cinnabar Pink (chalky and warm)
        return vec4f(0.88, 0.52, 0.55, 1.0);
    }

    // ── Slider spine (The "Madder Lake" fill) ──────────────────────────────────
    let isSpine = in.color.r > 0.24 && in.color.r < 0.35
               && in.color.g < 0.20 && in.color.b < 0.12;

    if isSpine {
        let fillY  = SPINE_MIN_Y + u.sliderValue * (SPINE_MAX_Y - SPINE_MIN_Y) + u.sliderPos.y;

        // Background: Muted, dark clay-grey
        // Fill: Desaturated terracotta pink
        let col = select(
            vec3f(0.22, 0.18, 0.18),         // Off-black with a hint of red-earth
            vec3f(0.75, 0.40, 0.42) * 0.9,   // Muted Renaissance Pink
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

    // Adjusted lamp to be slightly cooler (incense smoke/candlelight)
    let lampColor = vec3f(1.0, 0.92, 0.80);
    let ambient   = vec3f(0.08, 0.08, 0.12);

    return vec4f(in.color * (ambient + diffuse * lampColor), 1.0);
}