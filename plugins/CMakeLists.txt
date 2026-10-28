set(VITAL_DIR ${CMAKE_SOURCE_DIR}/vendor/vital)
set(VITAL_SRC ${VITAL_DIR}/src)

# ---- Vital sources -------------------------------------------------------------------------------
# Vital's headers are not self-contained: upstream builds everything as "unity" translation
# units with a specific include order. We keep that structure (core/unity/*.cpp), minus the
# parts that only make sense inside the full synth (voices, oscillators, modulation matrix,
# accounts, wavetable editing) and the full-synth engine; VialFX uses Vital's effects-only engine.
set(VITAL_CPP
  ${CMAKE_SOURCE_DIR}/core/unity/common.cpp
  ${CMAKE_SOURCE_DIR}/core/unity/synthesis.cpp
  ${CMAKE_SOURCE_DIR}/core/unity/interface_look_and_feel.cpp
  ${CMAKE_SOURCE_DIR}/core/unity/interface_editor_components.cpp
  ${CMAKE_SOURCE_DIR}/core/unity/interface_editor_sections.cpp
  ${CMAKE_SOURCE_DIR}/core/unity/interface_editor_sections2.cpp)

file(GLOB_RECURSE VITAL_DIRS LIST_DIRECTORIES true ${VITAL_SRC}/*)
set(VITAL_INCLUDE_DIRS ${VITAL_SRC} ${VITAL_DIR}/third_party)
foreach(d IN LISTS VITAL_DIRS)
  if(IS_DIRECTORY ${d})
    list(APPEND VITAL_INCLUDE_DIRS ${d})
  endif()
endforeach()
# Both engines ship a sound_engine.h; only the effects-only one may be visible.
list(FILTER VITAL_INCLUDE_DIRS EXCLUDE REGEX "/synthesis/synth_engine$")
# Our replacements must win over Vital's headers of the same name.
list(PREPEND VITAL_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/core)

# ---- Resources (fonts, icons, skin, tunings) -----------------------------------------------------
file(GLOB VITAL_FONTS ${VITAL_DIR}/fonts/*.ttf ${VITAL_DIR}/fonts/*.otf)
file(GLOB VITAL_ICONS ${VITAL_DIR}/icons/*.svg)
file(GLOB VITAL_TUNINGS ${VITAL_DIR}/tunings/*.scl)
juce_add_binary_data(vitalfx_resources
  SOURCES ${VITAL_DIR}/default.vitalskin ${VITAL_FONTS} ${VITAL_ICONS} ${VITAL_TUNINGS})
set_target_properties(vitalfx_resources PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- Our own code --------------------------------------------------------------------------------
set(VITALFX_CORE_CPP)  # compiled inside core/unity/interface_editor_sections2.cpp

# Dev convenience: -DVITALFX_ONLY="Reverb;Delay" builds just those plugins.
set(VITALFX_ONLY "" CACHE STRING "Limit the build to these plugin targets (default: all)")

# vitalfx_add_plugin(NAME <target> EFFECT <EffectKind index> CODE <4 chars> [WIDTH n] [HEIGHT n])
function(vitalfx_add_plugin)
  cmake_parse_arguments(P "" "NAME;EFFECT;CODE;WANTS_MIDI" "" ${ARGN})
  if(VITALFX_ONLY AND NOT "${P_NAME}" IN_LIST VITALFX_ONLY)
    return()
  endif()

  set(midi FALSE)
  if(P_WANTS_MIDI)
    set(midi TRUE)
  endif()

  juce_add_plugin(${P_NAME}
    VERSION 1.5.5  # Vital's version string: its preset loader refuses presets newer than this
    COMPANY_NAME "VialFX"
    PLUGIN_MANUFACTURER_CODE VtFx
    PLUGIN_CODE ${P_CODE}
    FORMATS ${VITALFX_FORMATS}
    PRODUCT_NAME "Vial ${P_NAME}"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT ${midi}
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx)

  juce_generate_juce_header(${P_NAME})

  target_sources(${P_NAME} PRIVATE ${VITAL_CPP} ${VITALFX_CORE_CPP})
  target_include_directories(${P_NAME} PRIVATE ${VITAL_INCLUDE_DIRS})

  target_compile_definitions(${P_NAME} PRIVATE
    VITALFX_EFFECT=${P_EFFECT}
    NO_AUTH=1
    NO_TEXT_ENTRY=0
    JUCE_OPENGL3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_DSP_USE_SHARED_FFTW=0)

  target_link_libraries(${P_NAME} PRIVATE
    vitalfx_resources
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_opengl
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i.86")
    if(NOT MSVC)
      target_compile_options(${P_NAME} PRIVATE -msse2)
    endif()
  endif()

  # Vital's code predates these warnings being useful; keep the build output readable.
  if(MSVC)
    target_compile_options(${P_NAME} PRIVATE /w /bigobj)
  else()
    target_compile_options(${P_NAME} PRIVATE -w)
  endif()

  if(VITALFX_ENABLE_CLAP)
    clap_juce_extensions_plugin(TARGET ${P_NAME}
      CLAP_ID "com.vialfx.${P_NAME}"
      CLAP_FEATURES audio-effect)
  endif()
endfunction()

# EffectKind index -> plugin. Four-character plugin codes must stay unique and stable.
vitalfx_add_plugin(NAME Chorus     EFFECT 0 CODE VfCh)
vitalfx_add_plugin(NAME Compressor EFFECT 1 CODE VfCm)
vitalfx_add_plugin(NAME Delay      EFFECT 2 CODE VfDl)
vitalfx_add_plugin(NAME Distortion EFFECT 3 CODE VfDs)
vitalfx_add_plugin(NAME EQ         EFFECT 4 CODE VfEq)
vitalfx_add_plugin(NAME Filter     EFFECT 5 CODE VfFl WANTS_MIDI TRUE)
vitalfx_add_plugin(NAME Flanger    EFFECT 6 CODE VfFg)
vitalfx_add_plugin(NAME Phaser     EFFECT 7 CODE VfPh)
vitalfx_add_plugin(NAME Reverb     EFFECT 8 CODE VfRv)
