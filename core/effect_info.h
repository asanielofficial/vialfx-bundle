// VialFX - single-effect plugins built from Vital's effect modules.
// Copyright (C) the VialFX contributors. Licensed under GPL-3.0-or-later.
// Based on Vital by Matt Tytel (https://github.com/mtytel/vital), GPL-3.0.
#pragma once

#include <string>
#include <vector>

namespace vitalfx {

enum class EffectKind { kChorus, kCompressor, kDelay, kDistortion, kEq, kFilter, kFlanger, kPhaser, kReverb };

struct EffectInfo {
  EffectKind kind;
  const char* display_name;     // shown in the host and the window title
  const char* section_title;    // title Vital's section draws for itself
  const char* control_prefix;   // prefix of Vital control names that belong to this effect
  const char* on_control;       // Vital's per-effect power control
  const char* preset_folder;    // folder name under the user's VialFX preset directory
  bool is_filter;               // filter plugin gets the keytrack setting and a MIDI input
};

inline const EffectInfo& getEffectInfo(EffectKind kind) {
  static const EffectInfo kInfos[] = {
    { EffectKind::kChorus,     "Chorus",     "CHORUS",     "chorus_",     "chorus_on",     "Chorus",     false },
    { EffectKind::kCompressor, "Compressor", "COMPRESSOR", "compressor_", "compressor_on", "Compressor", false },
    { EffectKind::kDelay,      "Delay",      "DELAY",      "delay_",      "delay_on",      "Delay",      false },
    { EffectKind::kDistortion, "Distortion", "DISTORTION", "distortion_", "distortion_on", "Distortion", false },
    { EffectKind::kEq,         "EQ",         "EQ",         "eq_",         "eq_on",         "EQ",         false },
    { EffectKind::kFilter,     "Filter",     "FILTER",     "filter_fx_",  "filter_fx_on",  "Filter",     true  },
    { EffectKind::kFlanger,    "Flanger",    "FLANGER",    "flanger_",    "flanger_on",    "Flanger",    false },
    { EffectKind::kPhaser,     "Phaser",     "PHASER",     "phaser_",     "phaser_on",     "Phaser",     false },
    { EffectKind::kReverb,     "Reverb",     "REVERB",     "reverb_",     "reverb_on",     "Reverb",     false },
  };
  return kInfos[static_cast<int>(kind)];
}

// Which effect this binary is. Set per plugin target by CMake (VITALFX_EFFECT=<EffectKind index>).
#ifndef VITALFX_EFFECT
  #define VITALFX_EFFECT 8
#endif

inline const EffectInfo& thisEffect() { return getEffectInfo(static_cast<EffectKind>(VITALFX_EFFECT)); }

}  // namespace vitalfx
