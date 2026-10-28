// VialFX - Licensed under GPL-3.0-or-later. Based on Vital by Matt Tytel (GPL-3.0).
#pragma once

#include "JuceHeader.h"

#include <memory>

#include "effect_info.h"
#include "synth_section.h"
#include "synth_types.h"

namespace vitalfx {

// Builds the exact section Vital shows for this effect in its effects tab.
std::unique_ptr<SynthSection> createEffectSection(EffectKind kind, const vital::output_map& mono_modulations);

}  // namespace vitalfx
