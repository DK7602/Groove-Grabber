cmake_minimum_required(VERSION 3.24)

project(GrooveGrabber VERSION 1.0.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

option(GG_BUILD_TESTS "Build the headless test + screenshot runner" ON)

# ------------------------------------------------------------------------------------------------
# JUCE (fetched automatically; set FETCHCONTENT_SOURCE_DIR_JUCE to a local copy to skip the download)
# ------------------------------------------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
FetchContent_Declare(juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.10
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(juce)

# ------------------------------------------------------------------------------------------------
# Plug-in
# ------------------------------------------------------------------------------------------------
set(GG_FORMATS Standalone VST3)
if(APPLE)
    list(APPEND GG_FORMATS AU)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND GG_FORMATS LV2)
endif()

juce_add_plugin(GrooveGrabber
    COMPANY_NAME                 "Grabber Audio"
    COMPANY_WEBSITE              "https://github.com/DK7602/Groove-Grabber"
    PRODUCT_NAME                 "Groove Grabber"
    BUNDLE_ID                    com.grabberaudio.groovegrabber
    PLUGIN_MANUFACTURER_CODE     Grbr
    PLUGIN_CODE                  GrG1
    FORMATS                      ${GG_FORMATS}
    IS_SYNTH                     TRUE
    NEEDS_MIDI_INPUT             TRUE
    NEEDS_MIDI_OUTPUT            FALSE
    IS_MIDI_EFFECT               FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS  TRUE
    COPY_PLUGIN_AFTER_BUILD      FALSE
    VST3_CATEGORIES              Instrument Drum Sampler
    AU_MAIN_TYPE                 kAudioUnitType_MusicDevice
    LV2URI                       "https://github.com/DK7602/Groove-Grabber"
    ICON_BIG                     "${CMAKE_CURRENT_SOURCE_DIR}/Resources/icon.png")

juce_add_binary_data(GrooveGrabberBinaryData
    HEADER_NAME GGBinaryData.h
    NAMESPACE   GGBinary
    SOURCES
        Resources/Fonts/Orbitron-Black.ttf
        Resources/Fonts/Orbitron-Bold.ttf
        Resources/Fonts/Rajdhani-Medium.ttf
        Resources/Fonts/Rajdhani-SemiBold.ttf
        Resources/Fonts/Rajdhani-Bold.ttf
        Resources/badge_256.png
        Resources/THIRD_PARTY_NOTICES.txt)

set(GG_CORE_SOURCES
    Source/core/SoundTypes.cpp
    Source/core/SoundInfo.cpp
    Source/core/Analyzer.cpp
    Source/core/Library.cpp
    Source/core/Styles.cpp
    Source/core/References.cpp
    Source/core/SongContext.cpp
    Source/core/Kit.cpp
    Source/core/KitGenerator.cpp
    Source/core/SampleRender.cpp
    Source/core/Grooves.cpp
    Source/core/KitPlayer.cpp
    Source/core/Exporter.cpp
    Source/core/Harvester.cpp
    Source/core/SongAnalysis.cpp
    Source/core/Settings.cpp)

set(GG_UI_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/StandaloneBridge.cpp
    Source/ui/Theme.cpp
    Source/ui/Widgets.cpp
    Source/ui/HeaderBar.cpp
    Source/ui/BuildPage.cpp
    Source/ui/KitViews.cpp
    Source/ui/LibraryPage.cpp
    Source/ui/KitsPage.cpp
    Source/ui/ExportPanel.cpp)

add_library(GGShared INTERFACE)
target_include_directories(GGShared INTERFACE Source)
target_link_libraries(GGShared INTERFACE
    GrooveGrabberBinaryData
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

set(GG_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(GG_LINUX_MINIMAL "Disable ALSA/JACK/Xrandr/Xinerama/Xcursor (CI / headless builds)" OFF)
    if(GG_LINUX_MINIMAL)
        list(APPEND GG_DEFS JUCE_ALSA=0 JUCE_JACK=0 JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0)
    endif()
endif()

target_compile_definitions(GGShared INTERFACE ${GG_DEFS} GG_VERSION="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(GGShared INTERFACE /bigobj /utf-8)
endif()

target_sources(GrooveGrabber PRIVATE ${GG_CORE_SOURCES} ${GG_UI_SOURCES})
target_link_libraries(GrooveGrabber PRIVATE GGShared)
target_compile_definitions(GrooveGrabber PUBLIC JUCE_MODAL_LOOPS_PERMITTED=0)

# ------------------------------------------------------------------------------------------------
# Headless test runner: unit tests for the engine + UI screenshots
# ------------------------------------------------------------------------------------------------
if(GG_BUILD_TESTS)
    juce_add_console_app(GGTests PRODUCT_NAME "GGTests")
    target_sources(GGTests PRIVATE Tests/TestMain.cpp Tests/TestSounds.cpp Tests/TestScreens.cpp ${GG_CORE_SOURCES} ${GG_UI_SOURCES})
    target_compile_definitions(GGTests PRIVATE GG_TEST_RUNNER=1 JUCE_MODAL_LOOPS_PERMITTED=1
        GG_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Tests")
    target_link_libraries(GGTests PRIVATE GGShared)
endif()
