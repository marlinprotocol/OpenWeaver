cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(beacon VERSION 0.0.1 LANGUAGES CXX)

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

add_library(beacon INTERFACE)
add_library(marlin::beacon ALIAS beacon)

target_include_directories(beacon
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(beacon INTERFACE cxx_std_17)

# libsodium
target_link_libraries(beacon INTERFACE sodium::sodium)

# marlin::net
target_link_libraries(beacon INTERFACE marlin::net)

# spdlog
target_link_libraries(beacon INTERFACE spdlog::spdlog)

install(TARGETS beacon
	EXPORT marlin-beacon-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-beacon-export
	FILE
		MarlinBeaconTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinBeacon
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinBeaconConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinBeacon
)


##########################################################
# Tests
##########################################################

enable_testing()
# GTest
include("${CMAKE_CURRENT_LIST_DIR}/cmake/gtest.cmake")

set(TEST_SOURCES
)

add_custom_target(beacon_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main beacon)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(beacon_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_custom_target(beacon_examples)

add_executable(discovery_example
	examples/discovery.cpp
)
add_dependencies(beacon_examples discovery_example)

target_include_directories(discovery_example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)
target_link_libraries(discovery_example PUBLIC beacon stdc++fs)
target_compile_options(discovery_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)

add_executable(server
	examples/server.cpp
)
add_dependencies(beacon_examples server)

target_include_directories(server
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)
target_link_libraries(server PUBLIC beacon)
target_compile_options(server PRIVATE -Werror -Wall -Wextra -pedantic-errors)

##########################################################
# All
##########################################################

add_custom_target(beacon_all)
add_dependencies(beacon_all net beacon_tests beacon_examples)
