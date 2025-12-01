cmake_minimum_required(VERSION 3.22)
project(PlaitsVST VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
)
FetchContent_MakeAvailable(JUCE)

# Plugin target
juce_add_plugin(PlaitsVST
    COMPANY_NAME "MasseyIS"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Msis
    PLUGIN_CODE Plvt
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "PlaitsVST"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.png")

target_sources(PlaitsVST PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/PresetManager.cpp
    src/dsp/stmlib/dsp/units.cc
    src/dsp/stmlib/utils/random.cc
    src/dsp/plaits/resources.cc
    src/dsp/plaits/dsp/voice.cc
    src/dsp/plaits/dsp/engine/additive_engine.cc
    src/dsp/plaits/dsp/engine/bass_drum_engine.cc
    src/dsp/plaits/dsp/engine/chord_engine.cc
    src/dsp/plaits/dsp/engine/fm_engine.cc
    src/dsp/plaits/dsp/engine/grain_engine.cc
    src/dsp/plaits/dsp/engine/hi_hat_engine.cc
    src/dsp/plaits/dsp/engine/modal_engine.cc
    src/dsp/plaits/dsp/engine/noise_engine.cc
    src/dsp/plaits/dsp/engine/particle_engine.cc
    src/dsp/plaits/dsp/engine/snare_drum_engine.cc
    src/dsp/plaits/dsp/engine/speech_engine.cc
    src/dsp/plaits/dsp/engine/string_engine.cc
    src/dsp/plaits/dsp/engine/swarm_engine.cc
    src/dsp/plaits/dsp/engine/virtual_analog_engine.cc
    src/dsp/plaits/dsp/engine/waveshaping_engine.cc
    src/dsp/plaits/dsp/engine/wavetable_engine.cc
    src/dsp/plaits/dsp/engine2/chiptune_engine.cc
    src/dsp/plaits/dsp/engine2/phase_distortion_engine.cc
    src/dsp/plaits/dsp/engine2/six_op_engine.cc
    src/dsp/plaits/dsp/engine2/string_machine_engine.cc
    src/dsp/plaits/dsp/engine2/virtual_analog_vcf_engine.cc
    src/dsp/plaits/dsp/engine2/wave_terrain_engine.cc
    src/dsp/plaits/dsp/speech/lpc_speech_synth.cc
    src/dsp/plaits/dsp/speech/lpc_speech_synth_controller.cc
    src/dsp/plaits/dsp/speech/lpc_speech_synth_phonemes.cc
    src/dsp/plaits/dsp/speech/lpc_speech_synth_words.cc
    src/dsp/plaits/dsp/speech/naive_speech_synth.cc
    src/dsp/plaits/dsp/speech/sam_speech_synth.cc
    src/dsp/plaits/dsp/physical_modelling/modal_voice.cc
    src/dsp/plaits/dsp/physical_modelling/resonator.cc
    src/dsp/plaits/dsp/physical_modelling/string.cc
    src/dsp/plaits/dsp/physical_modelling/string_voice.cc
    src/dsp/plaits/dsp/fm/algorithms.cc
    src/dsp/plaits/dsp/fm/dx_units.cc
    src/dsp/plaits/dsp/chords/chord_bank.cc
    src/dsp/envelope.cpp
    src/dsp/resampler.cpp
    src/dsp/voice.cpp
    src/dsp/voice_allocator.cpp)

target_include_directories(PlaitsVST PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/stmlib)

target_compile_definitions(PlaitsVST PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    STMLIB_X86=1)

target_link_libraries(PlaitsVST PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_basics)

# Tests
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(PlaitsVSTTests
    test/dsp/VoiceTests.cpp
    test/dsp/VoiceAllocatorTests.cpp
    test/dsp/EnvelopeTests.cpp
    test/dsp/ResamplerTests.cpp
    test/dsp/PlaitsEngineTests.cpp
    src/dsp/stmlib/dsp/units.cc
    src/dsp/stmlib/utils/random.cc
    src/dsp/plaits/resources.cc
    src/dsp/plaits/dsp/voice.cc
    src/dsp/plaits/dsp/engine/additive_engine.cc
    src/dsp/plaits/dsp/engine/bass_drum_engine.cc
    src/dsp/plaits/dsp/engine/chord_engine.cc
    src/dsp/plaits/dsp/engine/fm_engine.cc
    src/dsp/plaits/dsp/engine/grain_engine.cc
    src/dsp/plaits/dsp/engine/hi_hat_engine.cc
    src/dsp/plaits/dsp/engine/modal_engine.cc
    src/dsp/plaits/dsp/engine/noise_engine.cc
    src/dsp/plaits/dsp/engine/particle_engine.cc
    src/dsp/plaits/dsp/engine/snare_drum_engine.cc
    src/dsp/plaits/dsp/engine/speech_engine.cc
    src/dsp/plaits/dsp/engine/string_engine.cc
    src/dsp/plaits/dsp/engine/swarm_engine.cc
    src/dsp/plaits/dsp/engine/virtual_analog_engine.cc
    src/dsp/plaits/dsp/engine/waveshaping_engine.cc
    src/dsp/plaits/dsp/engine/wavetable_engine.cc
    src/dsp/plaits/dsp/engine2/chiptune_engine.cc
    src/dsp/plaits/dsp/engine2/phase_distortion_engine.cc
    src/dsp/plaits/dsp/engine2/six_op_engine.cc
    src/dsp/plaits/dsp/engine2/string_machine_engine.cc
    src/dsp/plaits/dsp/engine2/virtual_analog_vcf_engine.cc
    src/dsp/plaits/dsp/engine2/wave_terrain_engine.cc
    src/dsp/plaits/dsp/speech/lpc_speech_synth.cc
    src/dsp/plaits/dsp/speech/lpc_speech_synth_controller.cc
    src/dsp/plaits/dsp/speech/lpc_speech_synth_phonemes.cc
    src/dsp/plaits/dsp/speech/lpc_speech_synth_words.cc
    src/dsp/plaits/dsp/speech/naive_speech_synth.cc
    src/dsp/plaits/dsp/speech/sam_speech_synth.cc
    src/dsp/plaits/dsp/physical_modelling/modal_voice.cc
    src/dsp/plaits/dsp/physical_modelling/resonator.cc
    src/dsp/plaits/dsp/physical_modelling/string.cc
    src/dsp/plaits/dsp/physical_modelling/string_voice.cc
    src/dsp/plaits/dsp/fm/algorithms.cc
    src/dsp/plaits/dsp/fm/dx_units.cc
    src/dsp/plaits/dsp/chords/chord_bank.cc
    src/dsp/envelope.cpp
    src/dsp/resampler.cpp
    src/dsp/voice.cpp
    src/dsp/voice_allocator.cpp)

target_include_directories(PlaitsVSTTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/dsp/stmlib)

target_compile_definitions(PlaitsVSTTests PRIVATE STMLIB_X86=1)

target_link_libraries(PlaitsVSTTests PRIVATE gtest_main)

include(GoogleTest)
gtest_discover_tests(PlaitsVSTTests)
