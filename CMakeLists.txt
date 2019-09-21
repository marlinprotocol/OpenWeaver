cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(pubsub VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(pubsub INTERFACE)

target_include_directories(pubsub
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

#MarlinStream
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-stream.cmake")
target_link_libraries(pubsub INTERFACE Marlin::stream)

#MarlinNet
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-net.cmake")
target_link_libraries(pubsub INTERFACE Marlin::net)

# Spdlog
include("${CMAKE_CURRENT_LIST_DIR}/cmake/spdlog.cmake")
target_link_libraries(pubsub INTERFACE spdlog::spdlog)

install(TARGETS pubsub
	EXPORT marlin-pubsub-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-pubsub-export
  FILE
	MarlinPubsubTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/MarlinPubsub
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinPubsubConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MarlinPubsub
)

##########################################################
# Build examples
##########################################################

add_executable(testclient
	examples/client.cpp
)
target_link_libraries(testclient PUBLIC pubsub)
