// VialFX - Licensed under GPL-3.0-or-later. Based on Vital by Matt Tytel (GPL-3.0).
#include "effect_section_factory.h"

#include "chorus_section.h"
#include "compressor_section.h"
#include "delay_section.h"
#include "distortion_section.h"
#include "equalizer_section.h"
#include "filter_section.h"
#include "flanger_section.h"
#include "phaser_section.h"
#include "pitch_shifter_section.h"
#include "utility_section.h"
#include "freq_shifter_section.h"
#include "vibrato_section.h"
#include "tremolo_section.h"
#include "reverb_section.h"

namespace vitalfx {

std::unique_ptr<SynthSection> createEffectSection(EffectKind kind, const vital::output_map& mono) {
  switch (kind) {
    case EffectKind::kChorus:     return std::make_unique<ChorusSection>("CHORUS", mono);
    case EffectKind::kCompressor: return std::make_unique<CompressorSection>("COMPRESSOR");
    case EffectKind::kDelay:      return std::make_unique<DelaySection>("DELAY", mono);
    case EffectKind::kDistortion: return std::make_unique<DistortionSection>("DISTORTION", mono);
    case EffectKind::kEq:         return std::make_unique<EqualizerSection>("EQ", mono);
    case EffectKind::kFilter:     return std::make_unique<FilterSection>("fx", mono);
    case EffectKind::kFlanger:    return std::make_unique<FlangerSection>("FLANGER", mono);
    case EffectKind::kPhaser:     return std::make_unique<PhaserSection>("PHASER", mono);
    case EffectKind::kReverb:     return std::make_unique<ReverbSection>("REVERB", mono);
    case EffectKind::kPitchShifter: return std::make_unique<PitchShifterSection>("PITCH SHIFTER");
    case EffectKind::kUtility:    return std::make_unique<UtilitySection>("UTILITY");
    case EffectKind::kFreqShifter: return std::make_unique<FreqShifterSection>("FREQ SHIFTER");
    case EffectKind::kVibrato: return std::make_unique<VibratoSection>("VIBRATO");
    case EffectKind::kTremolo: return std::make_unique<TremoloSection>("TREMOLO");
    case EffectKind::kMorph:      return nullptr;   // built by FullInterface (it needs the engine and the wavetables)
  }
  return nullptr;
}

}  // namespace vitalfx
