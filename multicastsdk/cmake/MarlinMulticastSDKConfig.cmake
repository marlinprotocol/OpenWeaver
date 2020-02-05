get_filename_component(MULTICASTSDK_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${MULTICASTSDK_CMAKE_DIR})

find_package(MarlinPubsub REQUIRED)
find_package(MarlinBeacon REQUIRED)
find_package(MarlinNet REQUIRED)
find_package(MarlinStream REQUIRED)
find_package(MarlinLpf REQUIRED)
find_package(spdlog REQUIRED)

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Marlin::multicastsdk)
    include("${MULTICASTSDK_CMAKE_DIR}/MarlinMulticastSDKTargets.cmake")
endif()
