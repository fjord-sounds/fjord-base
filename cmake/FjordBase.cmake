# Build helpers shared by every fjord-sounds plugin.
#
#   fjord_setup_juce()          JUCE 8: a local checkout (FJORD_JUCE_DIR, default fjord-base/external/JUCE) or the pinned tag
#   fjord_add_base_library()    the fjord_base interface library (+ FjordBaseData: fonts and effect presets)
#   fjord_add_plugin(...)       an instrument or effect plugin with its RenderTest tool, see below
#
# Plugins never call juce_add_plugin directly: formats, codes, binary data, warnings and the test tool are set up the
# same way everywhere.

set(FJORD_BASE_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." CACHE INTERNAL "fjord-base source folder")
get_filename_component(FJORD_BASE_ROOT "${FJORD_BASE_ROOT}" ABSOLUTE)

set(FJORD_JUCE_TAG "8.0.4" CACHE STRING "JUCE release used when no local checkout exists")
set(FJORD_JUCE_DIR "${FJORD_BASE_ROOT}/external/JUCE" CACHE PATH "Local JUCE checkout (optional)")
option(FJORD_COPY_PLUGIN "Install the plugins to the user plugin folders after building" ON)

file(STRINGS "${FJORD_BASE_ROOT}/VERSION" _fjord_base_version LIMIT_COUNT 1)
set(FJORD_BASE_VERSION "${_fjord_base_version}" CACHE INTERNAL "fjord-base version")

# Plugin formats per platform: VST3 + Standalone everywhere, AU on macOS, LV2 on Linux.
# (cache variables: fjord-base is added with add_subdirectory, plain variables would not reach the plugin's scope)
set(_formats VST3 Standalone)
if(APPLE)
    list(APPEND _formats AU)
elseif(UNIX)
    list(APPEND _formats LV2)
endif()
set(FJORD_FORMATS ${_formats} CACHE INTERNAL "Plugin formats of this platform")

set(FJORD_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    CACHE INTERNAL "Compile definitions of every plugin")

function(fjord_git_hash dir out)
    execute_process(COMMAND git rev-parse --short HEAD WORKING_DIRECTORY "${dir}"
                    OUTPUT_VARIABLE hash OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    set(${out} "${hash}" PARENT_SCOPE)
endfunction()

macro(fjord_setup_juce)
    if(NOT TARGET juce::juce_core)
        if(EXISTS "${FJORD_JUCE_DIR}/CMakeLists.txt")
            add_subdirectory("${FJORD_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
        else()
            include(FetchContent)
            FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                                      GIT_TAG ${FJORD_JUCE_TAG} GIT_SHALLOW TRUE)
            FetchContent_MakeAvailable(JUCE)
        endif()
    endif()
endmacro()

function(fjord_add_base_library)
    if(TARGET fjord_base)
        return()
    endif()

    # Fonts (Manrope, SIL OFL) and the factory presets of every effect, shared by all plugins.
    file(GLOB fonts CONFIGURE_DEPENDS "${FJORD_BASE_ROOT}/resources/fonts/*.ttf")
    file(GLOB fx_presets CONFIGURE_DEPENDS "${FJORD_BASE_ROOT}/presets/fx/*.fjordmod" "${FJORD_BASE_ROOT}/presets/chains/*.fjordfx")
    juce_add_binary_data(FjordBaseData NAMESPACE FjordBaseData HEADER_NAME FjordBaseData.h SOURCES ${fonts} ${fx_presets})
    set_target_properties(FjordBaseData PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

    # An interface library: the sources compile inside every plugin (JUCE modules must only be built once per target).
    file(GLOB_RECURSE base_sources CONFIGURE_DEPENDS "${FJORD_BASE_ROOT}/source/fjord_base/*.cpp")
    file(GLOB_RECURSE base_headers CONFIGURE_DEPENDS "${FJORD_BASE_ROOT}/source/fjord_base/*.h")
    add_library(fjord_base INTERFACE)
    target_sources(fjord_base INTERFACE ${base_sources} ${base_headers})
    target_include_directories(fjord_base INTERFACE "${FJORD_BASE_ROOT}/source")
    fjord_git_hash("${FJORD_BASE_ROOT}" base_hash)
    target_compile_definitions(fjord_base INTERFACE FJORD_BASE_VERSION="${FJORD_BASE_VERSION}"
                                                    $<$<BOOL:${base_hash}>:FJORD_BASE_GIT_HASH="${base_hash}">)
    target_link_libraries(fjord_base INTERFACE FjordBaseData juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats)
endfunction()

# fjord_add_plugin(<Target>
#     PRODUCT_NAME "Torden"  CODE Trdn  BUNDLE_ID com.fjord.torden
#     [EFFECT]                                   # an audio effect instead of an instrument
#     [VST3_CATEGORIES Instrument Synth]
#     [PRESETS "presets/Factory/*.torden" ...]  # globs (relative to the plugin) embedded into BinaryData
#     [RESOURCES files ...]                      # further files for BinaryData
#     [SOURCE_DIR Source] [SOURCES files ...]    # sources (default: everything below Source/)
#     [INCLUDES dirs ...] [DEFINITIONS defs ...] [LINK targets ...]
#     [RENDER_TEST tests/RenderTest.cpp]         # offline test tool <Target>RenderTest (default if the file exists)
#     [JUCE_ARGS ...])                           # extra juce_add_plugin arguments (e.g. MICROPHONE_PERMISSION_ENABLED TRUE)
function(fjord_add_plugin target)
    cmake_parse_arguments(P "EFFECT" "PRODUCT_NAME;CODE;BUNDLE_ID;SOURCE_DIR;RENDER_TEST;ICON"
                          "VST3_CATEGORIES;PRESETS;RESOURCES;SOURCES;INCLUDES;DEFINITIONS;LINK;JUCE_ARGS" ${ARGN})

    if(NOT P_PRODUCT_NAME)
        set(P_PRODUCT_NAME "${target}")
    endif()
    if(NOT P_SOURCE_DIR)
        set(P_SOURCE_DIR "Source")
    endif()
    if(NOT P_ICON AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.png")
        set(P_ICON "resources/icon.png")
    endif()
    if(NOT P_RENDER_TEST AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/RenderTest.cpp")
        set(P_RENDER_TEST "tests/RenderTest.cpp")
    endif()
    string(TOLOWER "${P_PRODUCT_NAME}" lower_name)
    string(REPLACE " " "-" repo_name "${lower_name}")

    if(P_EFFECT)
        set(kind_args IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE AU_MAIN_TYPE kAudioUnitType_Effect)
        if(NOT P_VST3_CATEGORIES)
            set(P_VST3_CATEGORIES Fx)
        endif()
    else()
        set(kind_args IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE AU_MAIN_TYPE kAudioUnitType_MusicDevice)
        if(NOT P_VST3_CATEGORIES)
            set(P_VST3_CATEGORIES Instrument)
        endif()
    endif()

    set(icon_args)
    if(P_ICON)
        set(icon_args ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/${P_ICON}")
    endif()

    juce_add_plugin(${target}
        COMPANY_NAME "Fjord"
        BUNDLE_ID "${P_BUNDLE_ID}"
        PLUGIN_MANUFACTURER_CODE Fjrd
        PLUGIN_CODE ${P_CODE}
        FORMATS ${FJORD_FORMATS}
        LV2URI "https://github.com/fjord-sounds/${repo_name}"
        PRODUCT_NAME "${P_PRODUCT_NAME}"
        VERSION ${PROJECT_VERSION}
        ${icon_args}
        ${kind_args}
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES ${P_VST3_CATEGORIES}
        COPY_PLUGIN_AFTER_BUILD ${FJORD_COPY_PLUGIN}
        ${P_JUCE_ARGS})

    if(P_SOURCES)
        set(sources ${P_SOURCES})
    else()
        file(GLOB_RECURSE sources CONFIGURE_DEPENDS
             "${CMAKE_CURRENT_SOURCE_DIR}/${P_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/${P_SOURCE_DIR}/*.h")
    endif()
    target_sources(${target} PRIVATE ${sources})
    set(includes "${CMAKE_CURRENT_SOURCE_DIR}/${P_SOURCE_DIR}" ${P_INCLUDES})
    target_include_directories(${target} PRIVATE ${includes})

    # Factory presets and further resources are embedded (namespace BinaryData, like every JUCE project).
    set(data_files ${P_RESOURCES})
    foreach(pattern IN LISTS P_PRESETS)
        file(GLOB_RECURSE found CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${pattern}")
        list(APPEND data_files ${found})
    endforeach()
    if(NOT data_files)
        # BinaryData must exist (an empty placeholder keeps the code uniform)
        set(placeholder "${CMAKE_CURRENT_BINARY_DIR}/${target}_placeholder.txt")
        file(WRITE "${placeholder}" "fjord")
        set(data_files "${placeholder}")
    endif()
    juce_add_binary_data(${target}BinaryData SOURCES ${data_files})
    set_target_properties(${target}BinaryData PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

    fjord_git_hash("${CMAKE_CURRENT_SOURCE_DIR}" hash)
    set(definitions ${FJORD_DEFINITIONS} ${P_DEFINITIONS})
    if(hash)
        list(APPEND definitions FJORD_GIT_HASH="${hash}")
    endif()
    target_compile_definitions(${target} PUBLIC ${definitions})
    target_link_libraries(${target}
        PRIVATE ${target}BinaryData fjord_base ${P_LINK}
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

    # Offline test and tool binary (renders, screenshots, loudness, parameter table): no DAW needed.
    if(P_RENDER_TEST)
        set(test ${target}RenderTest)
        juce_add_console_app(${test} PRODUCT_NAME "${test}")
        target_sources(${test} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${P_RENDER_TEST}" ${sources})
        target_include_directories(${test} PRIVATE ${includes})
        target_compile_definitions(${test} PRIVATE ${definitions}
            JucePlugin_Name="${P_PRODUCT_NAME}" JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_Build_Standalone=0
            FJORD_RENDER_TEST=1)
        target_link_libraries(${test} PRIVATE ${target}BinaryData fjord_base ${P_LINK} juce::juce_recommended_config_flags)
        if(APPLE)
            set_target_properties(${test} PROPERTIES OSX_ARCHITECTURES arm64)
        endif()
    endif()
endfunction()
