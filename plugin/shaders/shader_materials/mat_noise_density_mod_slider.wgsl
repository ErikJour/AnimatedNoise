//===============================================
//Noise Density Mod Slider Shader
//===============================================
fn vertexDensityModSlider(pos: ptr<function, vec3f>, color: vec3f) -> vec4f {
        let sliderPosition = u.sliderPosition;
        let reducedRatio = 0.5;
        let reduced = vec3f(
                    pos.x * reducedRatio,
                    pos.y * reducedRatio,
                    pos.z * reducedRatio
                );
        let worldPosition = vec4f(reduced + sliderPosition, 1.0);
        *pos = worldPosition.xyz;
        return projectPerspective(worldPosition.xyz);
}

fn fragmentDensityModSlider(in: VertexOutput) -> vec4f {
    return shadeSpineTube(in);
}



