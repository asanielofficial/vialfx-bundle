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
  ${CMAKE_SOURCE_DIR}/core/unity/interface_editor_sections2.cpp
  ${CMAKE_SOURCE_DIR}/core/unity/interface_wavetable.cpp)

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
  SOURCES ${VITAL_DIR}/default.vialskin ${VITAL_FONTS} ${VITAL_ICONS} ${VITAL_TUNINGS})
set_target_properties(vitalfx_resources PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ---- Our own code --------------------------------------------------------------------------------
set(VITALFX_CORE_CPP)  # compiled inside core/unity/interface_editor_sections2.cpp

# Dev convenience: -DVITALFX_ONLY="Reverb;Delay" builds just those plugins.
set(VITALFX_ONLY "" CACHE STRING "Limit the build to these plugin targets (default: all)")

# vitalfx_add_plugin(NAME <target> EFFECT <EffectKind index> CODE <4 chars> [WIDTH n] [HEIGHT n])
function(vitalfx_add_plugin)
  cmake_parse_arguments(P "" "NAME;EFFECT;CODE;WANTS_MIDI;PRODUCT;MIDI_CC" "" ${ARGN})
  # MIDI_CC: let VST3 hosts deliver pitch bend, pressure and CC74 (JUCE emulates them with hidden parameters).
  # Off by default; the MPE-aware Resonator turns it on.
  set(midi_cc 0)
  if(P_MIDI_CC)
    set(midi_cc 1)
  endif()
  if(NOT P_PRODUCT)
    set(P_PRODUCT "${P_NAME}")
  endif()
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
    PRODUCT_NAME "Vial ${P_PRODUCT}"
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
    JUCE_OPENGL3=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=${midi_cc}
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
    target_compile_options(${P_NAME} PRIVATE /W0 /bigobj)
    # MSVC does not define __SSE2__, and JUCE's juce_dsp.h then defines it as an EMPTY macro, which
    # breaks Vital's "#elif __SSE2__" in poly_values.h (error C1017). Vital's own Windows project
    # defines it as 1 on the command line, so JUCE leaves it alone. Same for the aligned-storage
    # switch that Vital's SIMD types need with MSVC's standard containers.
    target_compile_definitions(${P_NAME} PRIVATE __SSE2__=1 _ENABLE_EXTENDED_ALIGNED_STORAGE)
  else()
    target_compile_options(${P_NAME} PRIVATE -w)
  endif()

  if(VITALFX_ENABLE_CLAP)
    clap_juce_extensions_plugin(TARGET ${P_NAME}
      CLAP_ID "com.vialfx.${P_NAME}"
      CLAP_FEATURES audio-effect)
  endif()

  if(VITALFX_ENABLE_LV2)
    set(lv2_name ${P_NAME}_LV2)
    # LV2 plugin class (what hosts show in their plugin menus).
    set(lv2_class UtilityPlugin)
    foreach(pair Chorus:ChorusPlugin Compressor:CompressorPlugin Delay:DelayPlugin Distortion:DistortionPlugin
                 EQ:EQPlugin Filter:FilterPlugin Flanger:FlangerPlugin Phaser:PhaserPlugin Reverb:ReverbPlugin
                 PitchShifter:PitchPlugin FreqShifter:SpectralPlugin Vibrato:ModulatorPlugin Tremolo:ModulatorPlugin
                 Morph:WaveshaperPlugin Resonator:FilterPlugin)
      string(REPLACE ":" ";" kv ${pair})
      list(GET kv 0 k)
      list(GET kv 1 v)
      if(P_NAME STREQUAL k)
        set(lv2_class ${v})
      endif()
    endforeach()
    set(lv2_bundle "${CMAKE_CURRENT_BINARY_DIR}/${P_NAME}_artefacts/$<CONFIG>/LV2/Vial ${P_PRODUCT}.lv2")
    set(lv2_base "Vial_${P_NAME}")
    add_library(${lv2_name} MODULE ${juce_SOURCE_DIR}/modules/juce_audio_plugin_client/LV2/juce_LV2_Wrapper.cpp)
    target_include_directories(${lv2_name} PRIVATE $<TARGET_PROPERTY:${P_NAME},INCLUDE_DIRECTORIES> ${CMAKE_SOURCE_DIR}/tools/lv2_include)
    target_compile_definitions(${lv2_name} PRIVATE
      JucePlugin_Build_LV2=1
      JucePlugin_LV2URI="urn:vialfx:${P_NAME}"
      JucePlugin_LV2Category="${lv2_class}"
      JucePlugin_MaxNumInputChannels=2
      JucePlugin_MaxNumOutputChannels=2
      JucePlugin_WantsLV2Presets=0
      JucePlugin_WantsLV2TimePos=0
      JucePlugin_WantsLV2State=1)
    target_link_libraries(${lv2_name} PRIVATE ${P_NAME})
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
      target_link_libraries(${lv2_name} PRIVATE "-Wl,--no-undefined")
    endif()
    set_target_properties(${lv2_name} PROPERTIES
      PREFIX "" OUTPUT_NAME ${lv2_base}
      LIBRARY_OUTPUT_DIRECTORY "${lv2_bundle}"
      LIBRARY_OUTPUT_DIRECTORY_RELEASE "${lv2_bundle}"
      RUNTIME_OUTPUT_DIRECTORY "${lv2_bundle}"
      RUNTIME_OUTPUT_DIRECTORY_RELEASE "${lv2_bundle}")
    if(APPLE)
      # The wrapper writes "<name>.dylib" into the .ttl files on macOS.
      set_target_properties(${lv2_name} PROPERTIES SUFFIX ".dylib")
    endif()
    add_custom_command(TARGET ${lv2_name} POST_BUILD
      COMMAND $<TARGET_FILE:vialfx_lv2_ttl_gen> $<TARGET_FILE:${lv2_name}> ${lv2_base}
      WORKING_DIRECTORY "${lv2_bundle}"
      VERBATIM)
    add_dependencies(${lv2_name} vialfx_lv2_ttl_gen)
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
vitalfx_add_plugin(NAME PitchShifter EFFECT 9 CODE VfPs PRODUCT "Pitch Shifter")
vitalfx_add_plugin(NAME Utility     EFFECT 10 CODE VfUt)
vitalfx_add_plugin(NAME FreqShifter EFFECT 11 CODE VfFq PRODUCT "Freq Shifter")
vitalfx_add_plugin(NAME Vibrato     EFFECT 12 CODE VfVb)
vitalfx_add_plugin(NAME Tremolo     EFFECT 13 CODE VfTm)
vitalfx_add_plugin(NAME Morph       EFFECT 14 CODE VfMp)
vitalfx_add_plugin(NAME Resonator   EFFECT 15 CODE VfRs WANTS_MIDI TRUE MIDI_CC TRUE)
