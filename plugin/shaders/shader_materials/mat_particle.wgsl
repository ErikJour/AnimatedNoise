
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

fn noiseCloudShape(worldPos: vec3f, t: f32, time: f32) -> vec3f {
    let cloud = vec3f(
        worldPos.x + sin(time + worldPos.x * 25.0) * 0.005,
        worldPos.y,
        worldPos.z + sin(time + worldPos.x * 25.0) * 0.01
    );

    let GA          = 2.3999632;
    let yF          = 1.0 - 2.0 * t;
    let rF          = sqrt(max(0.0, 1.0 - yF * yF));
    let theta       = GA * t * 500.0 + time * 0.1;
    let shell       = vec3f(cos(theta) * rF, yF, sin(theta) * rF) * 0.2;
    let crystal     = (0.5 + 0.5 * sin(time * 0.35)) * 0.7;
    let noiseBall   = mix(cloud * 0.5, shell * 0.1, crystal * 0.7);
    return (noiseBall);
}

@vertex
fn vs_particle(in: ParticleVertexInput) -> ParticleVertexOutput {
    var out: ParticleVertexOutput;

    let worldPos = in.pos_size.xyz;
    let size     = in.pos_size.w * 0.15;
    let t        = in.life_vel.x;

    let shaped = noiseCloudShape(worldPos, t, u.time);
    let scaled = 0.065;

    let depthZ = 0.15;
    let halfH  = depthZ * tan(1.0 * 0.5);
    let halfW  = halfH * u.aspectRatio;
    let margin = 0.07;
    let cloudAnchor = vec3f(halfW - margin + 0.05, -(halfH - margin + 0.06), -depthZ);

    let viewPos     = shaped * scaled + cloudAnchor;
    let cornerView  = viewPos + vec3f(in.cornerOffset * size, 0.0);

    out.position  = u.projMatrix * vec4f(cornerView, 1.0);
    out.viewDepth = -viewPos.z;
    out.color     = in.color;
    out.uv        = in.uv;
    out.life      = in.life_vel.x;
    return out;
}

const kWorldCloudCentre = vec3f(0.0, 0.1875, 0.0);
const kWorldCloudScale  = 3.1415 * 0.85;
const kWorldCloudDot    = 2.4;
const kWorldCloudFloorY = -0.145;
const kWorldCloudSpeed  = 0.25;

@vertex
fn vs_particle_world(in: ParticleVertexInput) -> ParticleVertexOutput {
    var out: ParticleVertexOutput;

    let worldPos = in.pos_size.xyz;
    let size     = in.pos_size.w * kWorldCloudDot;
    let t        = in.life_vel.x;

    let shaped = noiseCloudShape(worldPos, t, u.time * kWorldCloudSpeed);
    let centre = kWorldCloudCentre + shaped * kWorldCloudScale;

    let camRight = u.modelMatrix[0].xyz;
    let camUp    = u.modelMatrix[1].xyz;
    var world    = centre
                 + camRight * (in.cornerOffset.x * size)
                 + camUp    * (in.cornerOffset.y * size);

    world.y = max(world.y, kWorldCloudFloorY) - 0.35;

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
    let alpha               = glow * mix(0.75, 0.55, depthFade) * in.color.a;

    return vec4f(in.color.r * u.resonate, in.color.g, in.color.b * u.sliderValue, alpha * 0.33);

}