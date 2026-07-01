fn shadePlane(in: VertexOutput) -> vec4f {
    let normal = computeNormal(in.worldPos);
    let baseColor = vec3f(0.0, 1.0, 0.0);
    return vec4f(baseColor * pointLight(in.worldPos, normal), 1.0);
}

fn vsPlane(pos: ptr<function, vec3f>) -> vec4f {
    let worldPos = u.modelMatrix * vec4f(*pos, 1.0);
    return projectPerspective(worldPos.xyz);
}

