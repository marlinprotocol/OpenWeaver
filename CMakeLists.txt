cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(stream VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

IF(APPLE)
    # Fix linking on 10.14+. See https://stackoverflow.com/questions/54068035
    LINK_DIRECTORIES(/usr/local/lib)
ENDIF()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(stream INTERFACE)

target_include_directories(stream
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# set_target_properties(stream PROPERTIES
# 	OUTPUT_NAME "marlin-stream"
# )

# MarlinNet
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-net.cmake")
target_link_libraries(stream INTERFACE Marlin::net)

# spdlog
include("${CMAKE_CURRENT_LIST_DIR}/cmake/spdlog.cmake")

install(TARGETS stream
	EXPORT marlin-stream-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/stream)

##########################################################
# Export
##########################################################

install(EXPORT marlin-stream-export
  FILE
	MarlinStreamTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinStreamConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)


##########################################################
# Tests
##########################################################

enable_testing()
find_package(GTest REQUIRED)
#include("${CMAKE_CURRENT_LIST_DIR}/cmake/gtest.cmake")

set(TEST_SOURCES
	test/testAckRanges.cpp
)

foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main Marlin::net stream)
	target_include_directories(${TEST_NAME}
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/include
	)
	add_test(${TEST_NAME} ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_executable(stream_example
	examples/stream.cpp
)

target_include_directories(stream_example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)

target_link_libraries(stream_example PUBLIC stream)
