
const TEXT_GREY: vec3f = vec3f(0.651, 0.690, 0.682);

fn fragmentText(in: VertexOutput) -> vec4f {
    let normal = computeNormal(in.worldPos);
    return vec4f(TEXT_GREY, 1.0);
}

fn vertexText(pos: vec3f) -> vec4f {
        return u.projMatrix * u.modelMatrix * vec4f(pos, 1.0);

}

