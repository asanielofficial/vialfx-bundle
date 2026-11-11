// VitalFX - first include of every unity translation unit.
// Mirrors Vital's own unity builds, which start from synth_section.h: Vital's headers are not
// self-contained and depend on this include order.
#pragma once

// Morph is the one plugin with wavetable and curve editors; the others do not compile them. (Morph = effect 14.)
#if defined(VITALFX_EFFECT) && VITALFX_EFFECT == 14
  #define VITALFX_HAS_MORPH 1
  #ifndef VITAL_FX
    #define VITAL_FX 1
  #endif
#else
  #define VITALFX_HAS_MORPH 0
  #ifndef VITAL_FX
    #define VITAL_FX 0
  #endif
#endif

#include "JuceHeader.h"
#if !HEADLESS
#include "synth_section.h"
#endif
