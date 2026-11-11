// VitalFX unity translation unit, derived from Vital's src/unity_build/interface_wavetable.cpp (GPL-3.0-or-later).
// Only Morph has wavetable editors.
#include "vitalfx_prelude.h"

#if VITALFX_HAS_MORPH
#include "frequency_filter_overlay.cpp"
#include "slew_limiter_overlay.cpp"
#include "wave_warp_overlay.cpp"
#include "file_source_overlay.cpp"
#include "wave_fold_overlay.cpp"
#include "phase_modifier_overlay.cpp"
#include "wave_line_source_overlay.cpp"
#include "wave_source_overlay.cpp"
#include "wave_window_overlay.cpp"
#include "wavetable_component_overlay.cpp"
#include "wavetable_component_list.cpp"
#include "wave_source_editor.cpp"
#include "phase_editor.cpp"
#include "wave_window_editor.cpp"
#include "bar_editor.cpp"
#include "wavetable_organizer.cpp"
#include "wavetable_overlay_factory.cpp"
#include "wavetable_playhead.cpp"
#include "wavetable_playhead_info.cpp"
#endif
