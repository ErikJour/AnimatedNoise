const TEXT_DARK_GREY: vec3f = vec3f(0.184, 0.310, 0.310);

fn vsTooltipText(pos: vec3f) -> vec4f {
        let posScale = vec3f(-0.03, -0.105, -0.2);
        let sizeScale = 0.008;

    // Perform component-wise multiplication
        let scaledPos = (pos * sizeScale) + posScale;
        return u.projMatrix * u.modelMatrix * vec4f((scaledPos), 1.0);

}

fn shadeTooltipText(in: VertexOutput) -> vec4f {
    let normal = computeNormal(in.worldPos);
    return vec4f(TEXT_DARK_GREY, 1.0);
}
