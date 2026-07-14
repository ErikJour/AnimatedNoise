fn vsMasterGainSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {

    let sliderPosition = u.sliderPosition;
    let worldPosition = vec4f(*pos + sliderPosition, 1.0);
    *pos = worldPosition.xyz;
    return projectPerspective(worldPosition.xyz);
}

fn shadeMasterGainSlider(in: VertexOutput) -> vec4f {
 let normal    = normalize(in.normal);
    let baseColor = vec3f(0.4, 0.5, 0.8);
    let uv = (in.worldPos.xz);
    let grain = filmGrain(uv, 0.01);
    let grainAmount = 0.1;
    let color = baseColor * (1.0 + grain * grainAmount);
    return vec4f(color, 1.0);
}
