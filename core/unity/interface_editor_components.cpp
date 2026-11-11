// VitalFX unity translation unit, derived from Vital's src/unity_build/interface_editor_components.cpp (GPL-3.0-or-later).
// Vital's headers are not self-contained; they rely on this include order.
#include "vitalfx_prelude.h"

#if VITALFX_HAS_MORPH
#include "lfo_editor.cpp"
#include "line_map_editor.cpp"
#endif
// The Custom filter model (every plugin has the filter section) draws its curves with the line editor.
#include "line_editor.cpp"
#include "synth_button.cpp"
#include "synth_preset_selector.cpp"
#include "preset_selector.cpp"
#include "tab_selector.cpp"
#if VITALFX_HAS_MORPH
#include "wavetable_3d.cpp"
#endif
#include "bar_renderer.cpp"
#include "open_gl_image.cpp"
#include "open_gl_image_component.cpp"
#include "open_gl_multi_image.cpp"
#include "open_gl_multi_quad.cpp"
#include "open_gl_component.cpp"
#include "open_gl_line_renderer.cpp"
#include "modulation_button.cpp"
#include "modulation_tab_selector.cpp"
#include "equalizer_response.cpp"
#include "oscilloscope.cpp"
#include "peak_meter_viewer.cpp"
#include "compressor_editor.cpp"
#include "filter_response.cpp"
#include "text_selector.cpp"
#include "modulation_meter.cpp"
#include "tempo_selector.cpp"
#include "synth_slider.cpp"
#include "open_gl_background.cpp"
