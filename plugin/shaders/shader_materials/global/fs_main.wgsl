//===============================================
//Fragment Shader Shared
//===============================================
@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    switch u.materialId {
        case MAT_LEVEL:                   { return fragmentLevel(in);           }
        case MAT_SKYLIGHT:                { return fragmentSkylight(in);        }
        case MAT_LOGO:                    { return fragmentLogo(in);            }
        case MAT_TOOLTIP:                 { return fragmentTooltipText(in);     }
        case MAT_TEXT:                    { return fragmentText(in);            }
        //============================
        //Sliders
        //============================
        case MAT_NOISE_LEVEL_SLIDER:      { return fragmentNoiseLevelSlider(in);}
        case MAT_NOISE_LEVEL_MOD_SLIDER:  { return fragmentLpgRezSlider(in);    }
        case MAT_NOISE_DENS_SLIDER:       { return fragmentDensitySlider(in);   }
        case MAT_NOISE_DENS_MOD_SLIDER:   { return fragmentDensityModSlider(in);}
        case MAT_ATTACK_SLIDER:           { return fragmentAttackSlider(in);    }
        case MAT_DECAY_SLIDER:            { return fragmentDecaySlider(in);     }
        case MAT_SUSTAIN_SLIDER:          { return fragmentSustainSlider(in);   }
        case MAT_RELEASE_SLIDER:          { return fragmentReleaseSlider(in);   }
        case MAT_FILTER_ONE_SLIDER:       { return fragmentFilterOneSlider(in);  }
        case MAT_FILTER_TWO_SLIDER:       { return fragmentFilterTwoSlider(in);  }
        //============================
        //Utilities
        //============================
        case MAT_LIGHT_HELPER:            { return fragmentLightHelper(in);}
        default:                          { return vec4f(1.0, 0.0, 1.0, 1.0);}
    }
}
