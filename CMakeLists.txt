cmake_minimum_required(VERSION 3.22)

project(TMRXCompressor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 개발 중 빌드 결과물을 시스템 플러그인 폴더로 복사할지 여부
option(TMRX_COPY_PLUGIN_AFTER_BUILD "Copy built plugins to the system plugin folders" OFF)
option(TMRX_BUILD_TESTS "Build DSP unit tests" ON)

# ---------------------------------------------------------------------------
# JUCE
#   -DJUCE_DIR=/path/to/JUCE 로 로컬 체크아웃을 지정하거나,
#   지정하지 않으면 FetchContent 로 자동 다운로드합니다.
# ---------------------------------------------------------------------------
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

# ---------------------------------------------------------------------------
# AAX
#   AAX 는 Avid 에서 받은 AAX SDK 가 있어야 빌드됩니다.
#   -DAAX_SDK_PATH=/path/to/aax-sdk 를 주면 AAX 포맷이 활성화됩니다.
# ---------------------------------------------------------------------------
set(AAX_SDK_PATH "" CACHE PATH "Path to the Avid AAX SDK (optional)")

set(TMRX_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND TMRX_FORMATS AU)
endif()
if(AAX_SDK_PATH)
    juce_set_aax_sdk_path(${AAX_SDK_PATH})
    list(APPEND TMRX_FORMATS AAX)
    message(STATUS "AAX SDK found at ${AAX_SDK_PATH} - AAX format enabled")
else()
    message(STATUS "AAX_SDK_PATH not set - building without AAX")
endif()

juce_add_plugin(TMRXCompressor
    COMPANY_NAME                "TMRX Studio"
    PRODUCT_NAME                "TMRX Compressor"
    BUNDLE_ID                   com.tmrxstudio.compressor
    PLUGIN_MANUFACTURER_CODE    Tmrx
    PLUGIN_CODE                 Tcmp
    FORMATS                     ${TMRX_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Dynamics
    AAX_CATEGORY                AAX_ePlugInCategory_Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD     ${TMRX_COPY_PLUGIN_AFTER_BUILD})

target_sources(TMRXCompressor PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(TMRXCompressor PRIVATE Source)

target_compile_definitions(TMRXCompressor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(TMRXCompressor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Tests (JUCE 에 의존하지 않는 순수 DSP 코드만 테스트)
# ---------------------------------------------------------------------------
if(TMRX_BUILD_TESTS)
    enable_testing()
    add_executable(CompressorTests Tests/CompressorTests.cpp)
    target_include_directories(CompressorTests PRIVATE Source)
    add_test(NAME CompressorTests COMMAND CompressorTests)
endif()
