cmake_minimum_required(VERSION 3.22)

project(FLSampler VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Reuse local JUCE 8 source cache if available for instant builds
if(EXISTS "C:/Users/anton/PitchCanvas/build/_deps/juce-src/CMakeLists.txt")
    add_subdirectory("C:/Users/anton/PitchCanvas/build/_deps/juce-src"
                     "${CMAKE_CURRENT_BINARY_DIR}/JUCE"
                     EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(FLSampler
    COMPANY_NAME                "PAPIELGUAPO"
    PLUGIN_MANUFACTURER_CODE    Papi
    PLUGIN_CODE                 FlSm
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "FL Sampler"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             "Instrument" "Sampler"
)

juce_add_binary_data(FLSamplerAssets
    SOURCES
        Assets/Skin.jpg
)

target_sources(FLSampler
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/DSP/PrecomputedEffects.h
        Source/DSP/SamplerEngine.h
        Source/UI/FLLookAndFeel.h
        Source/UI/WAVScopeComponent.h
)

target_include_directories(FLSampler
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(FLSampler
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        _USE_MATH_DEFINES=1
)

target_link_libraries(FLSampler
    PRIVATE
        FLSamplerAssets
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
