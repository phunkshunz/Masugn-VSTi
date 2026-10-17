cmake_minimum_required(VERSION 3.22)

project(MASUGN VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: bygg in C++-runtime i pluginet
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# JUCE hämtas automatiskt vid bygget
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

# Testprogrammet (tools/) bygger bara ljudmotorn utan fönster: MASUGN_TOOLS=ON
option(MASUGN_TOOLS "Bygg bara testprogrammet för ljudmotorn" OFF)

if(MASUGN_TOOLS)
    juce_add_console_app(MasugnTest PRODUCT_NAME "MasugnTest")
    target_sources(MasugnTest PRIVATE tools/MasugnTest.cpp)
    target_include_directories(MasugnTest PRIVATE src)
    target_compile_definitions(MasugnTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(MasugnTest
        PRIVATE juce::juce_audio_formats juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    return()
endif()

juce_add_plugin(Masugn
    COMPANY_NAME "Phunkshunz AUDIO"
    PRODUCT_NAME "MASUGN"
    BUNDLE_ID "com.phunkshunz.masugn"
    PLUGIN_MANUFACTURER_CODE Phnk
    PLUGIN_CODE Masg
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU Standalone)

target_sources(Masugn
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp)

target_compile_definitions(Masugn
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Masugn
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Bild av panelen (bara för utveckling på Linux): MASUGN_SNAPSHOT=ON
option(MASUGN_SNAPSHOT "Bygg ett program som sparar en bild av panelen" OFF)
if(MASUGN_SNAPSHOT)
    juce_add_console_app(MasugnShot PRODUCT_NAME "MasugnShot")
    target_sources(MasugnShot PRIVATE tools/Snapshot.cpp)
    target_link_libraries(MasugnShot PRIVATE Masugn juce::juce_audio_utils)
endif()

# Linux används bara för utveckling och tester: inget ljudkort behövs
if(UNIX AND NOT APPLE AND TARGET Masugn)
    target_compile_definitions(Masugn PUBLIC JUCE_ALSA=0 JUCE_JACK=0)
endif()

# MSVC: läs källkoden som UTF-8
if(MSVC)
    if(TARGET Masugn)
        target_compile_options(Masugn PRIVATE /utf-8 /bigobj)
    endif()
endif()
