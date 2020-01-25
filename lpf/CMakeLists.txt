cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(lpf VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(lpf INTERFACE)
add_library(marlin::lpf ALIAS lpf)

target_include_directories(lpf
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# marlin::net
target_link_libraries(lpf INTERFACE marlin::net)

# spdlog
target_link_libraries(lpf INTERFACE spdlog::spdlog)

install(TARGETS lpf
	EXPORT marlin-lpf-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-lpf-export
	FILE
		MarlinLpfTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinLpf
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinLpfConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinLpf
)


##########################################################
# Tests
##########################################################

enable_testing()
include("${CMAKE_CURRENT_LIST_DIR}/cmake/gtest.cmake")

set(TEST_SOURCES
)

add_custom_target(lpf_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main lpf)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(lpf_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_custom_target(lpf_examples)

add_executable(lpf_example
	examples/lpf.cpp
)
add_dependencies(lpf_examples lpf_example)

target_link_libraries(lpf_example PUBLIC lpf)


##########################################################
# All
##########################################################

add_custom_target(lpf_all)
add_dependencies(lpf_all lpf lpf_tests lpf_examples)
