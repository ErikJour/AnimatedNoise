//===============================================
//Vertex Shader Shared
//===============================================
fn projectPerspective(worldPos: vec3f) -> vec4f {
    return u.viewProjMatrix * vec4f(worldPos, 1.0);
}

fn projectFlat(pos: vec3f) -> vec4f {
    return vec4f(pos.x, pos.y, pos.z * 0.5 + 0.5, 1.0);
}

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    var pos = in.position;

    switch u.materialId {
        case MAT_LEVEL:                   { out.clipPos  = vertexLevel              (&pos);           }
        case MAT_SKYLIGHT:                { out.clipPos  = vertexSkylight           (&pos);           }
        case MAT_LOGO:                    { out.clipPos  = vertexLogo               (pos);            }
        case MAT_TOOLTIP:                 { out.clipPos  = vertexTooltipText        (pos);            }
        case MAT_TEXT:                    { out.clipPos  = vertexText               (pos);            }
        //============================
        //Sliders
        //============================
        case MAT_NOISE_LEVEL_SLIDER:      { out.clipPos  = vertexNoiseLevelSlider   (&pos, in.color); }
        case MAT_NOISE_LEVEL_MOD_SLIDER:  { out.clipPos  = vertexLpgRezSlider       (&pos, in.color); }
        case MAT_NOISE_DENS_SLIDER:       { out.clipPos  = vertexDensitySlider      (&pos, in.color); }
        case MAT_NOISE_DENS_MOD_SLIDER:   { out.clipPos  = vertexDensityModSlider   (&pos, in.color); }
        case MAT_ATTACK_SLIDER:           { out.clipPos  = vertexAttackSlider       (&pos, in.color); }
        case MAT_DECAY_SLIDER:            { out.clipPos  = vertexDecaySlider        (&pos, in.color); }
        //============================
        //Utilities
        //============================
        case MAT_LIGHT_HELPER:            { out.clipPos  = vertexLightHelper        (&pos, in.color); }
        default:                          { out.clipPos  = projectPerspective       (pos);            }
    }

    out.color    = in.color;
    out.worldPos = pos;
    out.normal   = in.normal;
    return out;
}
