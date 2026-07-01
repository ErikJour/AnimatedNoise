fn vsFloor(pos: ptr<function, vec3f>) -> vec4f {
    let worldPosition = u.modelMatrix * vec4f(*pos, 1.0);
    *pos = worldPosition.xyz;
    return projectPerspective(worldPosition.xyz);
}

fn hash(p: vec2<f32>) -> f32 {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

fn filmGrain(uv: vec2<f32>, t: f32) -> f32 {
    let seed = 0.01;
    let p = uv * vec2(1024.0, 1024.0) + vec2(seed * 127.3, seed * 93.7);
    var grain = hash(p) * 2.0 - 1.0;
    grain = sign(grain) * pow(abs(grain), 1.5);
    return grain;
}

fn shadeFloor(in: VertexOutput) -> vec4f {
    let normal    = normalize(in.normal);
    let baseColor = vec3f(0.7, 0.75, 0.5);
    let uv = (in.worldPos.xz);
    let grain = filmGrain(uv, 0.01);
    let grainAmount = 0.1;
    let color = baseColor * (1.0 + grain * grainAmount);
    return vec4f(color * roomPointLight(in.worldPos, normal), 1.0);
}