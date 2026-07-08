
const CAVE_ALBEDO: vec3f = vec3f(0.58, 0.46, 0.38);   // warm sandstone

fn shadeCave(in: VertexOutput) -> vec4f {
    let normal = computeNormal(in.worldPos);
    return vec4f(CAVE_ALBEDO * pointLight(in.worldPos, normal), 1.0);
}

fn vsCave(pos: vec3f) -> vec4f {
//    return projectPerspective(pos);
        return u.projMatrix * u.modelMatrix * vec4f(pos, 1.0);

}
