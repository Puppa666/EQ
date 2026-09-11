cmake_minimum_required(VERSION 3.22)
project(BalancEQ VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# JUCE framework - automatic download via FetchContent
# ============================================================================
include(FetchContent)

set(JUCE_VERSION "7.0.12")
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        v${JUCE_VERSION}
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

# ============================================================================
# Plugin definition
# ============================================================================
juce_add_plugin(BalancEQ
    COMPANY_NAME "BalancEQ"
    PLUGIN_MANUFACTURER_CODE Blnc
    PLUGIN_CODE BlEQ
    FORMATS VST3
    PRODUCT_NAME "BalancEQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

target_sources(BalancEQ PRIVATE
    Source/PluginProcessor.cpp
)

target_include_directories(BalancEQ PRIVATE Source)

target_compile_definitions(BalancEQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_ALSA=0
    JUCE_JACK=0
    JUCE_ENABLE_LADSPA=0
    JUCE_ENABLE_LV2=0
    JUCE_ENABLE_VST3=1
    JUCE_ENABLE_AU=0
    JUCE_ENABLE_AAX=0
    JUCE_ENABLE_UNIT_TESTS=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# Link JUCE modules
target_link_libraries(BalancEQ PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_events
    juce::juce_core
)
