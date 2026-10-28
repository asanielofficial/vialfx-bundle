// VitalFX - first include of every unity translation unit.
// Mirrors Vital's own unity builds, which start from synth_section.h: Vital's headers are not
// self-contained and depend on this include order.
#pragma once
#include "JuceHeader.h"
#if !HEADLESS
#include "synth_section.h"
#endif
