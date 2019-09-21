get_filename_component(PUBSUB_CMAKE_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
include(CMakeFindDependencyMacro)

list(APPEND CMAKE_MODULE_PATH ${PUBSUB_CMAKE_DIR})

find_package(MarlinNet REQUIRED)
find_package(MarlinStream REQUIRED)
find_package(spdlog REQUIRED)

list(REMOVE_AT CMAKE_MODULE_PATH -1)

if(NOT TARGET Marlin::pubsub)
    include("${PUBSUB_CMAKE_DIR}/MarlinPubsubTargets.cmake")
endif()
