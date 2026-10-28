// VialFX - Licensed under GPL-3.0-or-later. Based on Vital by Matt Tytel (GPL-3.0).
#pragma once

#include "JuceHeader.h"

#include "effect_info.h"

namespace vitalfx {

// Plugin-level settings that are not Vital controls. They are stored next to the Vital state
// in the host session and are not automatable.
struct EffectSettings {
  bool keytrack_from_midi = true;  // filter plugin only: follow incoming MIDI notes for keytracking
};

// Folder where this effect's user presets live, e.g. ~/Documents/VialFX/Reverb/Presets
File getUserPresetDirectory(const EffectInfo& info);

}  // namespace vitalfx
