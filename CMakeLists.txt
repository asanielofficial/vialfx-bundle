cmake_minimum_required(VERSION 3.22)
project(VialFX-Bundle VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Current scope: VST3 on Windows and Linux. CLAP and macOS are wired up but off by default.
option(VITALFX_ENABLE_CLAP "Also build CLAP versions (needs clap-juce-extensions)" OFF)
option(VITALFX_ENABLE_MACOS "Allow macOS builds (untested)" OFF)

if(APPLE AND NOT VITALFX_ENABLE_MACOS)
  message(FATAL_ERROR "macOS builds are disabled for now. Pass -DVITALFX_ENABLE_MACOS=ON to try them.")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# Vital builds against a patched JUCE 6.0.5 (OpenGL transform feedback, Component and Button changes).
# We take upstream 6.0.5 for its CMake support and overlay Vital's modified modules on top.
FetchContent_Declare(juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 6.0.5
  GIT_SHALLOW ON
  PATCH_COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/vendor/vital/third_party/JUCE/modules <SOURCE_DIR>/modules)
FetchContent_MakeAvailable(juce)

if(VITALFX_ENABLE_CLAP)
  FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d)
  FetchContent_MakeAvailable(clap-juce-extensions)
endif()

set(VITALFX_FORMATS VST3)

add_subdirectory(plugins)

option(VITALFX_BUILD_TESTS "Build the small VST3 test host used in CI" ON)
if(VITALFX_BUILD_TESTS)
  juce_add_console_app(vialfx_host_test PRODUCT_NAME "vialfx_host_test")
  juce_generate_juce_header(vialfx_host_test)
  target_sources(vialfx_host_test PRIVATE tests/host_test.cpp)
  target_compile_definitions(vialfx_host_test PRIVATE
    JUCE_PLUGINHOST_VST3=1 JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  target_link_libraries(vialfx_host_test PRIVATE
    juce::juce_audio_processors juce::juce_audio_utils juce::juce_gui_extra
    juce::juce_recommended_config_flags)
endif()
