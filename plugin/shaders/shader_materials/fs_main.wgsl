//===============================================
//Fragment Shader Shared
//===============================================
@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    switch u.materialId {
        case MAT_TEXT:                    { return fragmentText(in);            }
        case MAT_MASTER_GAIN_SLIDER:      { return fragmentNoiseLevelSlider(in);}
        case MAT_PLANE:                   { return fragmentPlane(in);           }
        case MAT_LEVEL:                   { return fragmentLevel(in);           }
        case MAT_SKYLIGHT:                { return fragmentSkylight(in);        }
        case MAT_LPG_REZ_SLIDER:          { return fragmentLpgRezSlider(in);    }
        case MAT_NOIS_DENS_SLIDER:        { return fragmentDensitySlider(in);   }
        case MAT_LOGO:                    { return fragmentLogo(in);            }
        case MAT_TOOLTIP:                 { return fragmentTooltipText(in);     }

        default:                          { return vec4f(1.0, 0.0, 1.0, 1.0);}
    }
}
