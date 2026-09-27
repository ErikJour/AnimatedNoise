
struct ParticleVertexInput {
    @location(0) cornerOffset : vec2f,
    @location(1) uv           : vec2f,
    @location(2) pos_size     : vec4f,
    @location(3) color        : vec4f,
    @location(4) life_vel     : vec4f,
}

struct ParticleVertexOutput {
    @builtin(position) position  : vec4f,
    @location(0)       color     : vec4f,
    @location(1)       uv        : vec2f,
    @location(2)       life      : f32,
    @location(3)       viewDepth : f32,
}

const kNoiseLocation = vec3f(1.9, 0.4, 0.1);   // <-- the spot in world space you want
const kCloudRadius = 0.125;                      // world-space size of the cloud
const kCloudDot    = 0.05;                     // world-space particle size
const kTubeRadius   = 1.5;   // multiplier on p (p already spans ±0.15)


fn cloudLocal(p: vec3f, t: f32) -> vec3f {
    let GA    = 2.3999632;
    let yF    = 1.0 - 2.0 * t;
    let rF    = sqrt(max(0.0, 1.0 - yF * yF));
    let theta = GA * t * 5.0;
    let shell = vec3f(cos(theta) * rF, yF, sin(theta) * rF);
    return mix(p * kTubeRadius, shell, 0.89);
}

@vertex
fn vs_particle_world(in: ParticleVertexInput) -> ParticleVertexOutput {
    var out: ParticleVertexOutput;
    let t = in.life_vel.x;

    let centre = kNoiseLocation + cloudLocal(in.pos_size.xyz, t) * kCloudRadius;

    let camRight = normalize(vec3f(u.viewProjMatrix[0][0], u.viewProjMatrix[1][0], u.viewProjMatrix[2][0]));
    let camUp    = normalize(vec3f(u.viewProjMatrix[0][1], u.viewProjMatrix[1][1], u.viewProjMatrix[2][1]));
    let size     = in.pos_size.w * kCloudDot;
    let world    = centre + (camRight * in.cornerOffset.x + camUp * in.cornerOffset.y) * size;

    let clip      = projectPerspective(world);
    out.position  = clip;
    out.viewDepth = clip.w;
    out.color     = in.color;
    out.uv        = in.uv;
    out.life      = t;
    return out;
}

@fragment
fn fs_particle(in: ParticleVertexOutput) -> @location(0) vec4f {
    let distanceToCenter    = length(in.uv - 0.5);
    let glow                = clamp(0.05 / distanceToCenter - 0.1, 0.0, 1.0);
    let depthFade           = smoothstep(1.0, 1.1, in.viewDepth);
    let alpha               = glow * mix(0.75, 0.55, 1.0) * in.color.a;

    return vec4f(in.color.r * u.resonate, in.color.g, in.color.b * u.sliderValue, alpha * 0.33);
}