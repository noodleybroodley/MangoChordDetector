cmake_minimum_required(VERSION 3.16)
project(MangoChordDetector VERSION 0.1.0)

set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architecture" FORCE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory("../JUCE" JUCE)

# Create VST3 plugin
juce_add_plugin(MangoChordDetector
    PRODUCT_NAME "MangoChordDetector"
    COMPANY_NAME "BradleyHarrison"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE "Brad"
    PLUGIN_CODE "MnGo"
    VST3_CATEGORIES "Analyzer"
    FORMATS VST3)

target_compile_definitions(MangoChordDetector 
    PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0)

target_sources(MangoChordDetector PRIVATE
    Source/Main.cpp
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/NoteDetector.h
    Source/ChordDetector.h
    Source/MusicalDisplay.h)

set(JUCE_MODULES
    juce_gui_basics
    juce_audio_basics
    juce_audio_devices
    juce_audio_formats
    juce_audio_processors
    juce_audio_utils
    juce_dsp)

target_link_libraries(MangoChordDetector PRIVATE ${JUCE_MODULES})
target_include_directories(MangoChordDetector PRIVATE JuceLibraryCode Source)