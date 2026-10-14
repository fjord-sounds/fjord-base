# Single-effect plugins: every effect of the family as its own plugin ("Fjord Reverb" ...), built from the same
# generic processor and editor. tools/build-effects.sh builds, installs and validates them.
#
#   id          product name               code   (codes are stored by hosts – never change them)
set(FJORD_EFFECTS
    "saturation|Fjord Saturation|Fsat"
    "lofi|Fjord Lo-Fi|Flfi"
    "transient|Fjord Transient|Ftrs"
    "compressor|Fjord Compressor|Fcmp"
    "squash|Fjord Squash|Fsqh"
    "eq|Fjord EQ|Fequ"
    "filter|Fjord Filter|Fflt"
    "motion|Fjord Motion|Fmot"
    "delay|Fjord Delay|Fdly"
    "reverb|Fjord Reverb|Frvb"
    "width|Fjord Width|Fwdt"
    "tremolo|Fjord Tremolo|Ftrm"
    "shifter|Fjord Frequency Shifter|Fshf"
    "exciter|Fjord Exciter|Fexc"
    "harmonizer|Fjord Harmonizer|Fhrm"
    "out|Fjord Clipper|Fclp")

set(FJORD_EFFECTS_ONLY "" CACHE STRING "Build only these effects (comma separated ids, empty = all)")
string(REPLACE "," ";" only "${FJORD_EFFECTS_ONLY}")

file(GLOB effect_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")

set(index 0)
foreach(entry IN LISTS FJORD_EFFECTS)
    string(REPLACE "|" ";" parts "${entry}")
    list(GET parts 0 id)
    list(GET parts 1 name)
    list(GET parts 2 code)
    if(only AND NOT id IN_LIST only)
        math(EXPR index "${index} + 1")
        continue()
    endif()
    string(REPLACE " " "" target "${name}")
    string(REPLACE "-" "" target "${target}")
    fjord_add_plugin(${target}
        PRODUCT_NAME "${name}" CODE ${code} BUNDLE_ID "com.fjord.effect.${id}"
        EFFECT
        SOURCE_DIR source SOURCES ${effect_sources}
        DEFINITIONS FJORD_EFFECT_MODULE=${index}
        RENDER_TEST "")
    math(EXPR index "${index} + 1")
endforeach()

# One test tool for all effects: renders every effect and every preset, checks levels, makes screenshots.
juce_add_console_app(FjordEffectsTest PRODUCT_NAME "FjordEffectsTest")
target_sources(FjordEffectsTest PRIVATE tests/EffectsTest.cpp ${effect_sources})
target_include_directories(FjordEffectsTest PRIVATE source)
set(placeholder "${CMAKE_CURRENT_BINARY_DIR}/effects_placeholder.txt")
file(WRITE "${placeholder}" "fjord")
juce_add_binary_data(FjordEffectsTestData SOURCES "${placeholder}")
target_compile_definitions(FjordEffectsTest PRIVATE ${FJORD_DEFINITIONS} FJORD_EFFECT_MODULE=9 FJORD_RENDER_TEST=1
    JucePlugin_Name="Fjord Effects" JucePlugin_VersionString="${PROJECT_VERSION}" JucePlugin_Build_Standalone=0)
target_link_libraries(FjordEffectsTest PRIVATE FjordEffectsTestData fjord_base juce::juce_recommended_config_flags)
if(APPLE)
    set_target_properties(FjordEffectsTest PROPERTIES OSX_ARCHITECTURES arm64)
endif()
