fn vertexLightHelper(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
    let worldPos = u.modelMatrix * vec4f(*pos, 1.0);
    return projectPerspective(worldPos.xyz);
}

fn fragmentLightHelper(in: VertexOutput) -> vec4f {
    let normal = computeNormal(in.worldPos);
    let baseColor = vec3f(0.1, 0.1, 1.0);
    return vec4f(baseColor, 0.0);
}

