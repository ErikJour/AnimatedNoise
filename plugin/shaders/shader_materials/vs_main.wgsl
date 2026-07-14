// ── Projection helpers ────────────────────────────────────────────────────────
fn projectPerspective(worldPos: vec3f) -> vec4f {
    return u.viewProjMatrix * vec4f(worldPos, 1.0);
}

fn projectFlat(pos: vec3f) -> vec4f {
    return vec4f(pos.x, pos.y, pos.z * 0.5 + 0.5, 1.0);
}

// ── Vertex entry point ────────────────────────────────────────────────────────
@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    var pos = in.position;

    switch u.materialId {
        case MAT_TEXT:                    { out.clipPos  = vsText               (pos);            }
        case MAT_MASTER_GAIN_SLIDER:      { out.clipPos  = vsMasterGainSlider   (&pos, in.color); }
        case MAT_COMB_AMT_SLIDER:         { out.clipPos  = vsMasterGainSlider   (&pos, in.color); }
        case MAT_PLANE:                   { out.clipPos  = vsPlane              (&pos);           }
        case MAT_FLOOR:                   { out.clipPos  = vsFloor              (&pos);           }
        case MAT_SKYLIGHT:                { out.clipPos  = vsSkylight           (&pos);           }
        case MAT_LPG_REZ_SLIDER:          { out.clipPos  = vsLpgRezSlider       (&pos, in.color); }
        case MAT_NOIS_DENS_SLIDER:        { out.clipPos  = vsDensitySlider      (&pos, in.color); }
        case MAT_LOGO:                    { out.clipPos  = vsLogo               (pos);            }
        default:                          { out.clipPos  = projectPerspective   (pos);            }
    }

    out.color    = in.color;
    out.worldPos = pos;
    out.normal   = in.normal;
    return out;
}
