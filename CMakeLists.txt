cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(stream VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
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
target_compile_features(stream INTERFACE cxx_std_17)

# MarlinNet
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-net.cmake")
target_link_libraries(stream INTERFACE Marlin::net)

# spdlog
include("${CMAKE_CURRENT_LIST_DIR}/cmake/spdlog.cmake")
target_link_libraries(stream INTERFACE spdlog::spdlog)

install(TARGETS stream
	EXPORT marlin-stream-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-stream-export
  FILE
	MarlinStreamTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/MarlinStream
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinStreamConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MarlinStream
)

##########################################################
# Tests
##########################################################

enable_testing()
include("${CMAKE_CURRENT_LIST_DIR}/cmake/gtest.cmake")

set(TEST_SOURCES
	test/testAckRanges.cpp
)

foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main Marlin::net stream)
	add_test(${TEST_NAME} ${TEST_NAME})
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_executable(stream_example
	examples/stream.cpp
)
target_link_libraries(stream_example PUBLIC stream)
target_compile_options(stream_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(stream_example PRIVATE cxx_std_17)
