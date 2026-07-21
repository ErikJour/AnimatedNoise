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
        case MAT_TEXT:                    { out.clipPos  = vertexText               (pos);            }
        case MAT_MASTER_GAIN_SLIDER:      { out.clipPos  = vertexNoiseLevelSlider   (&pos, in.color); }
        case MAT_PLANE:                   { out.clipPos  = vertexPlane              (&pos);           }
        case MAT_LEVEL:                   { out.clipPos  = vertexLevel              (&pos);           }
        case MAT_SKYLIGHT:                { out.clipPos  = vertexSkylight           (&pos);           }
        case MAT_LPG_REZ_SLIDER:          { out.clipPos  = vertexLpgRezSlider       (&pos, in.color); }
        case MAT_NOIS_DENS_SLIDER:        { out.clipPos  = vertexDensitySlider      (&pos, in.color); }
        case MAT_LOGO:                    { out.clipPos  = vertexLogo               (pos);            }
        case MAT_TOOLTIP                  { out.clipPos  = vertexTooltipText        (pos);            }
        default:                          { out.clipPos  = projectPerspective       (pos);            }
    }

    out.color    = in.color;
    out.worldPos = pos;
    out.normal   = in.normal;
    return out;
}
