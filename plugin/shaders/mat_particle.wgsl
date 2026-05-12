// mat_particle.wgsl
// Slot 0 (per-vertex)   : QuadVertex  — cornerOffset (vec2f), uv (vec2f)
// Slot 1 (per-instance) : ParticleData — pos_size (vec4f), color (vec4f), life_vel (vec4f)

struct ParticleVertexInput {
    @location(0) cornerOffset : vec2f,
    @location(1) uv           : vec2f,
    @location(2) pos_size     : vec4f,
    @location(3) color        : vec4f,
    @location(4) life_vel     : vec4f,
}

struct ParticleVertexOutput {
    @builtin(position) position : vec4f,
    @location(0)       color    : vec4f,
    @location(1)       uv       : vec2f,
    @location(2)       life     : f32,
}

@vertex
fn vs_particle(in: ParticleVertexInput) -> ParticleVertexOutput {
    var out: ParticleVertexOutput;

    let worldPos = in.pos_size.xyz;
    let size     = in.pos_size.w;

    let animated = vec3f(
        worldPos.x,
        worldPos.y + sin(u.time + worldPos.x * 5.0) * 0.02,
        worldPos.z
    );

    let depth = animated.z + 1.0;
    let projX = animated.x * FOV_FACTOR / depth;  // reuse from common.wgsl
    let projY = animated.y * FOV_FACTOR / depth;

    let bx = in.cornerOffset.x * size / depth;
    let by = in.cornerOffset.y * size / depth;

    let zNDC = (depth - 0.01) / (100.0 - 0.01);

    out.position = vec4f(projX + bx, projY + by, zNDC, 1.0);
    out.color    = in.color;
    out.uv       = in.uv;
    out.life     = in.life_vel.x;

    return out;
}

@fragment
fn fs_particle(in: ParticleVertexOutput) -> @location(0) vec4f {
    let centered = (in.uv - vec2f(0.5)) * 2.0;
    let dist     = length(centered);
    let alpha    = smoothstep(1.0, 0.2, dist) * in.color.a * in.life;
    if alpha < 0.01 { discard; }
    return vec4f(in.color.rgb, alpha);
}