// ── Cave material ─────────────────────────────────────────────────────────────
fn shadeCave(in: VertexOutput) -> vec4f {
    let normal = computeNormal(in.worldPos);
    return vec4f(in.color * pointLight(in.worldPos, normal), 1.0);
}

fn vsCave(pos: vec3f) -> vec4f {
    return projectPerspective(pos);
}
