get_filename_component(RLPX_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${RLPX_CMAKE_DIR})

find_package(MarlinNet REQUIRED)
find_package(spdlog REQUIRED)
find_package(Snappy REQUIRED)

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Marlin::rlpx)
    include("${RLPX_CMAKE_DIR}/MarlinRlpxTargets.cmake")
endif()
