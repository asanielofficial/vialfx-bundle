// VitalFX unity translation unit, derived from Vital's src/unity_build/interface_editor_sections2.cpp (GPL-3.0-or-later).
// Vital's headers are not self-contained; they rely on this include order.
#include "vitalfx_prelude.h"

#include "equalizer_section.cpp"
#include "flanger_section.cpp"
#include "filter_section.cpp"
#include "distortion_section.cpp"
#include "full_interface.h"
#include "bank_exporter.cpp"
#include "preset_browser.cpp"
#include "popup_browser.cpp"
#include "header_section.cpp"
#include "delete_section.cpp"
#include "save_section.cpp"

// VitalFX code goes last so every Vital header is already in scope.
#include "../full_interface.cpp"
#include "../modulation_names.cpp"
#include "../settings_section.cpp"
#include "../effect_section_factory.cpp"
#include "../effect_header.cpp"
#include "../effect_state.cpp"
#include "../effect_plugin.cpp"
#include "../effect_editor.cpp"
