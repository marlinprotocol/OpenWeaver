get_filename_component(BEACON_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${BEACON_CMAKE_DIR})

# find_package(MarlinTest CONFIG REQUIRED COMPONENTS test
# 	NAMES "Marlin" CONFIGS "MarlinTestConfig.cmake")

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Marlin::beacon)
    include("${BEACON_CMAKE_DIR}/MarlinBeaconTargets.cmake")
endif()
