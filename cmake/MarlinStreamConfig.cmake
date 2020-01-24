get_filename_component(STREAM_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${STREAM_CMAKE_DIR})

find_package(MarlinNet REQUIRED)
find_package(spdlog REQUIRED)

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Marlin::stream)
    include("${STREAM_CMAKE_DIR}/MarlinStreamTargets.cmake")
endif()
