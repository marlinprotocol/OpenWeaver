cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(beacon VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(beacon INTERFACE)

target_include_directories(beacon
	INTERFACE
		$<INSTALL_INTERFACE:include/marlin/beacon>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# MarlinNet
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-net.cmake")
target_link_libraries(beacon INTERFACE Marlin::net)

# spdlog
include("${CMAKE_CURRENT_LIST_DIR}/cmake/spdlog.cmake")
target_link_libraries(beacon INTERFACE spdlog::spdlog)

install(TARGETS beacon
	EXPORT marlin-beacon-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/beacon)

##########################################################
# Export
##########################################################

install(EXPORT marlin-beacon-export
  FILE
	MarlinBeaconTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinBeaconConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)


##########################################################
# Tests
##########################################################

enable_testing()
find_package(GTest REQUIRED)

set(TEST_SOURCES
)

foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main beacon)
	target_include_directories(${TEST_NAME}
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/include
	)
	add_test(${TEST_NAME} ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_executable(discovery_example
	examples/main.cpp
)

target_include_directories(discovery_example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)

target_link_libraries(discovery_example PUBLIC beacon)

add_executable(server
	examples/server.cpp
)

target_include_directories(server
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)

target_link_libraries(server PUBLIC beacon)
