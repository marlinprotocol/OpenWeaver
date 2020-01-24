get_filename_component(LPF_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${LPF_CMAKE_DIR})

# find_package(MarlinTest CONFIG REQUIRED COMPONENTS test
# 	NAMES "Marlin" CONFIGS "MarlinTestConfig.cmake")

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Marlin::lpf)
    include("${LPF_CMAKE_DIR}/MarlinLpfTargets.cmake")
endif()
