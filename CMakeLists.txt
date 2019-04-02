cmake_minimum_required(VERSION 3.9)
project(pubsub VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(pubsub STATIC
	src/PubSubNode.cpp
)

target_include_directories(pubsub
	PUBLIC
		$<INSTALL_INTERFACE:include/marlin/pubsub>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(pubsub PROPERTIES
	OUTPUT_NAME "marlin-pubsub"
)

find_package(MarlinStream CONFIG REQUIRED COMPONENTS stream
	NAMES "Marlin" CONFIGS "MarlinStreamConfig.cmake")

find_package(spdlog REQUIRED)

install(TARGETS pubsub
	EXPORT marlin-pubsub-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/pubsub)

##########################################################
# Export
##########################################################

install(EXPORT marlin-pubsub-export
  FILE
	MarlinPubsubTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinPubsubConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)
