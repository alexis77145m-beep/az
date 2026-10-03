cmake_minimum_required(VERSION 3.22)
project(LSAFlanger VERSION 1.0.0)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(LSAFlanger
    COMPANY_NAME "AZ" PLUGIN_MANUFACTURER_CODE AzAz PLUGIN_CODE Lsaf
    FORMATS VST3 Standalone
    PRODUCT_NAME "LSA Flanger"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE)
target_sources(LSAFlanger PRIVATE Source/PluginProcessor.cpp)
target_compile_definitions(LSAFlanger PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(LSAFlanger PRIVATE juce::juce_audio_utils juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
