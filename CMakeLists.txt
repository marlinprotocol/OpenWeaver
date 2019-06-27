cmake_minimum_required(VERSION 3.9)
project(rlpx VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(rlpx STATIC
	src/RlpxCrypto.cpp
)

target_include_directories(rlpx
	PUBLIC
		$<INSTALL_INTERFACE:include/marlin/rlpx>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(rlpx PROPERTIES
	OUTPUT_NAME "marlin-rlpx"
)

target_link_libraries(rlpx PUBLIC cryptopp)

find_package(MarlinNet CONFIG REQUIRED COMPONENTS net
	NAMES "Marlin" CONFIGS "MarlinNetConfig.cmake")

target_link_libraries(rlpx PUBLIC Marlin::net)

find_package(spdlog REQUIRED)

find_package(Snappy REQUIRED)
target_link_libraries(rlpx PUBLIC Snappy::snappy)

install(TARGETS rlpx
	EXPORT marlin-rlpx-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/rlpx)

##########################################################
# Export
##########################################################

install(EXPORT marlin-rlpx-export
  FILE
	MarlinRlpxTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinRlpxConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

##########################################################
# Build examples
##########################################################

add_executable(rlpx_example
	examples/rlpx.cpp
)

target_include_directories(rlpx_example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)

target_link_libraries(rlpx_example PUBLIC rlpx)
