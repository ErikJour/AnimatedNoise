// ── Fragment entry point ──────────────────────────────────────────────────────
@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    switch u.materialId {
        case MAT_CAVE:                    { return shadeCave(in);            }
        case MAT_GLOBAL_GAIN_SLIDER:      { return shadeGainSlider(in);      }
        case MAT_COMB_AMT_SLIDER:         { return shadeCombSlider(in);      }
        case MAT_PLANE:                   { return shadePlane(in);           }
        case MAT_FLOOR:                   { return shadeFloor(in);           }
        case MAT_SKYLIGHT:                { return shadeSkylight(in);        }
        default:                          { return vec4f(1.0, 0.0, 1.0, 1.0);}
    }
}
