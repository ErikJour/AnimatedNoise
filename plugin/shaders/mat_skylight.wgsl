// ── Circular Skylight material ─────────────────────────────────────────────────

fn vsSkylight(pos: ptr<function, vec3f>) -> vec4f {
    let worldPos = u.modelMatrix * vec4f(*pos, 1.0);
    *pos = worldPos.xyz;                      // ← write back for lighting
    return projectPerspective(worldPos.xyz);
}

fn shadeSkylight(in: VertexOutput) -> vec4f {
    let normal    = normalize(in.normal);     // ← use vertex normal, not hardcoded
    let baseColor = vec3f(0.4, 0.15, 0.1);
    return vec4f(baseColor * pointLight(in.worldPos, normal), 1.0);
}