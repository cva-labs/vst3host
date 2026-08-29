cmake_minimum_required(VERSION 3.22)

project(VST3PlayerHost VERSION 1.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if (WIN32)
    # JUCE's ASIO backend requires the Steinberg ASIO SDK headers.
    FetchContent_Declare(
        ASIOSDK
        GIT_REPOSITORY https://github.com/audiosdk/asio.git
        GIT_TAG 496a0765b8bb9c26f764f22f9a9712a937177db2
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(ASIOSDK)
endif()

juce_add_gui_app(VST3PlayerHost
    PRODUCT_NAME "VST3 Player Host"
    COMPANY_NAME "CVA Labs"
    BUNDLE_ID "com.cvalabs.vst3playerhost"
    ICON_BIG Assets/vst3-player-host-icon.png
    ICON_SMALL Assets/vst3-player-host-icon.png)

juce_add_binary_data(HostAssets
    SOURCES Assets/cva-labs-logo.png)

target_sources(VST3PlayerHost PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/AudioEngine.cpp
    Source/AudioEngine.h)

if (WIN32)
    target_include_directories(VST3PlayerHost PRIVATE ${asiosdk_SOURCE_DIR}/common)
    target_link_libraries(VST3PlayerHost PRIVATE mfplat mfreadwrite mfuuid ole32)
endif()

target_compile_definitions(VST3PlayerHost PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_PLUGINHOST_VST3=1
    $<$<PLATFORM_ID:Windows>:JUCE_ASIO=1>
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0)

target_link_libraries(VST3PlayerHost PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_link_libraries(VST3PlayerHost PRIVATE HostAssets)

juce_generate_juce_header(VST3PlayerHost)

if (APPLE)
    set_target_properties(VST3PlayerHost PROPERTIES
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-")
endif()
