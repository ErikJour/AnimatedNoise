// ── Circular Floor material ─────────────────────────────────────────────────

fn vsFloor(pos: ptr<function, vec3f>) -> vec4f {
    let worldPos = u.modelMatrix * vec4f(*pos, 1.0);
    *pos = worldPos.xyz;
    return projectPerspective(worldPos.xyz);
}

fn shadeFloor(in: VertexOutput) -> vec4f {
    let normal    = normalize(in.normal);
    let baseColor = vec3f(0.4, 0.15, 0.1);
    return vec4f(baseColor * pointLight(in.worldPos, normal), 1.0);
}