fn vsMasterGainSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {

let expandedRatio = (u.pressed * 0.13) + 1.0 ;

    let expanded = vec3f(
            pos.x * expandedRatio,
            pos.y * expandedRatio,
            pos.z * expandedRatio
        );

    let sliderPosition = u.sliderPosition;
    let worldPosition  = vec4f(expanded + sliderPosition, 1.0);
    *pos               = worldPosition.xyz;

    return projectPerspective(worldPosition.xyz);
}

fn shadeMasterGainSlider(in: VertexOutput) -> vec4f {
    let normal      = normalize(in.normal);
    let baseColor   = vec3f(0.4, 0.5, 0.8);
    let uv          = (in.worldPos.xz);
    let grain       = filmGrain(uv, 0.01);
    let grainAmount = 0.1;
    let color       = baseColor * (1.0 + grain * grainAmount) ;
    let colorOut    = vec4(color, 1.0);
    let spine       = shadeSpineTube(in);

    return vec4f(spine.rgb * (1.0 + grain * grainAmount), spine.a);
}
