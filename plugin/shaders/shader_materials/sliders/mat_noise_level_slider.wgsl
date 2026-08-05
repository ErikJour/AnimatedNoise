fn vertexNoiseLevelSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {

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

fn fragmentNoiseLevelSlider(in: VertexOutput) -> vec4f {
    let normal      = normalize(in.normal);
    let baseColor   = vec3f(0.4, 0.5, 0.8);
    let uv          = (in.worldPos.xz);
    let grain       = filmGrain(uv, 0.01);
    let grainAmount = 0.1;
    //light experiment=====================================
    var light = vec3f(0.0);

    let viewDirection = normalize(u.cameraPosition - in.worldPos.xyz);

    light += ambientLight(in.worldPos.xyz,
                            normal,
                            vec3f(1.0, 0.0, 0.0),
                            0.2);

    let modelNormal = u.modelMatrix * vec4(normal, 0.0);
    light += directionalLight(in.worldPos.xyz,
                              modelNormal.xyz,
                              vec3f(0.1, 0.1, 0.1),
                               1.0,
                              vec3f(0.0, 1.0, 0.3),
                              viewDirection
                              );
    //Done==================================================
    let color       = baseColor * (1.0 + grain * grainAmount);
    let colorOut    = vec4(color, 1.0);
    let spine       = shadeSpineTube(in);

    return vec4f(spine.rgb * light * (1.0 + grain * grainAmount), spine.a);
}

fn shadeSpineTube(in: VertexOutput) -> vec4f {
    let cream       = vec3f(0.92, 0.86, 0.72);
    let grey        = vec3f(0.28, 0.27, 0.25);
    let v           = in.color.y;
    let isIndicator = in.color.z > 0.5;

    if (isIndicator) {
        let halfH           = 0.048;
        let indicatorCenter = clamp(u.sliderValue, halfH, 1.0 - halfH);
        let dCenter         = abs(v - indicatorCenter);
        let baseAlpha       = smoothstep(halfH, 0.0, dCenter);
        var alpha           = baseAlpha * 2.95;
        let rim             = 1.0 - abs(in.normal.y);

        let pulse           = sin(u.time * 4.0) * 0.15 + 0.85;
        let restColor       = vec3f(1.0, 0.38, 0.06);
        let pressedColor    = vec3f(1.0 * (1.0 - u.sliderValue), 0.82, 0.50);
        let sliderAmount    = f32(u.sliderValue + 0.3);
        let clampedSlider   = clamp(0.3, 1.0, sliderAmount);
        let indicatorColor  = mix(restColor * clampedSlider, pressedColor, u.pressed) * pulse
                            * (0.85 + 0.15 * rim * rim);
        let pressedAlpha    = mix(alpha, min(alpha * 1.25, 1.0), u.pressed);

        return vec4f(indicatorColor, pressedAlpha);
    }

    let pulse          = sin(u.time * 2.0 + v * 6.0) * 0.015 + 0.85;
    let fillEdge       = smoothstep(u.sliderValue - 0.03, u.sliderValue + 0.01, v);
    let pulseMask      = 1.0 - step(u.sliderValue, v);
    let animPulse      = mix(1.0, pulse, pulseMask);
    let creamWithPulse = cream * animPulse * 0.90;
    var finalColor     = mix(creamWithPulse, grey, fillEdge);

    finalColor        += cream;

    return vec4f(finalColor, 1.0);
}
