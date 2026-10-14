# Bootstrap snippet for plugin CMakeLists.txt files (copy it, do not include it – it runs before fjord-base exists).
#
#   set(FJORD_BASE_TAG "v0.2.0")
#   set(FJORD_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../fjord-base" CACHE PATH "Local fjord-base checkout")
#   if(EXISTS "${FJORD_BASE_DIR}/cmake/FjordBase.cmake")
#       add_subdirectory("${FJORD_BASE_DIR}" "${CMAKE_BINARY_DIR}/fjord-base")
#   else()
#       include(FetchContent)
#       FetchContent_Declare(fjord_base GIT_REPOSITORY https://github.com/fjord-sounds/fjord-base.git
#                                       GIT_TAG ${FJORD_BASE_TAG} GIT_SHALLOW TRUE)
#       FetchContent_MakeAvailable(fjord_base)
#   endif()
#
# Locally the sibling checkout ../fjord-base is used, so changes in fjord-base show up in every plugin at once.
# CI and everyone without that checkout get the pinned tag. After changing fjord-base: build and test all plugins,
# tag fjord-base, then raise FJORD_BASE_TAG in the plugins.
