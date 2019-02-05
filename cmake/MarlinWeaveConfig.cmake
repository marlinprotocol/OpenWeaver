get_filename_component(WEAVE_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${WEAVE_CMAKE_DIR})

find_package(MarlinFiber CONFIG REQUIRED COMPONENTS fiber
	NAMES "Marlin" CONFIGS "MarlinFiberConfig.cmake")

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Marlin::weave)
    include("${WEAVE_CMAKE_DIR}/MarlinWeaveTargets.cmake")
endif()
