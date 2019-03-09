get_filename_component(NET_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${NET_CMAKE_DIR})

find_package(LibUV MODULE REQUIRED COMPONENTS uv)

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Marlin::net)
    include("${NET_CMAKE_DIR}/MarlinNetTargets.cmake")
endif()
