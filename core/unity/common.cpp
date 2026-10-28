// VitalFX unity translation unit, derived from Vital's src/unity_build/common.cpp (GPL-3.0-or-later).
// Vital's headers are not self-contained; they rely on this include order.
#include "vitalfx_prelude.h"

#if !HEADLESS
#include "border_bounds_constrainer.cpp"
#endif

#include "line_generator.cpp"
#include "midi_manager.cpp"
#include "tuning.cpp"
#include "startup.cpp"
#include "synth_gui_interface.cpp"
#include "synth_parameters.cpp"
#include "load_save.cpp"
#include "synth_types.cpp"
#include "synth_base.cpp"
#include "wavetable_component_factory.cpp"
#include "wavetable_keyframe.cpp"
#include "file_source.cpp"
#include "shepard_tone_source.cpp"
#include "frequency_filter_modifier.cpp"
#include "wave_fold_modifier.cpp"
#include "phase_modifier.cpp"
#include "wavetable_creator.cpp"
#include "wave_line_source.cpp"
#include "wave_source.cpp"
#include "wavetable_group.cpp"
#include "wave_window_modifier.cpp"
#include "wavetable_component.cpp"
#include "pitch_detector.cpp"
#include "wave_warp_modifier.cpp"
#include "slew_limit_modifier.cpp"
