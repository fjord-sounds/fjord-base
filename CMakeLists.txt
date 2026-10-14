cmake_minimum_required(VERSION 3.22)

# fjord-base: the shared code of all fjord-sounds plugins (UI, effects, presets, build helpers).
#
# Plugins add it with the bootstrap snippet from cmake/bootstrap.cmake (a local ../fjord-base checkout, otherwise a
# pinned tag from GitHub). Built on its own, it builds the single-effect plugins ("Fjord Reverb" ...) and their test.

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")
    endif()
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" FJORD_BASE_VERSION LIMIT_COUNT 1)
    project(FjordBase VERSION ${FJORD_BASE_VERSION} LANGUAGES C CXX)
    set(FJORD_BASE_IS_TOP_LEVEL ON)
else()
    set(FJORD_BASE_IS_TOP_LEVEL OFF)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FjordBase.cmake")

fjord_setup_juce()
fjord_add_base_library()

if(FJORD_BASE_IS_TOP_LEVEL)
    add_subdirectory(effects)
endif()
