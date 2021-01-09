cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(simulator VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(simulator STATIC
	src/core/Simulator.cpp
)
add_library(marlin::simulator ALIAS simulator)

target_include_directories(simulator
	PUBLIC
		$<INSTALL_INTERFACE:include/marlin/simulator>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_options(simulator PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(simulator PUBLIC cxx_std_17)
target_link_libraries(simulator absl::container)

set_target_properties(simulator PROPERTIES
	OUTPUT_NAME "marlin-simulator"
)

install(TARGETS simulator
	EXPORT marlin-simulator-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-simulator-export
  FILE
	MarlinSimulatorTargets.cmake
  NAMESPACE
	marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/marlinSimulator
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinSimulatorConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinSimulator
)


##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES
	test/core/testEvent.cpp
	test/core/testIndexedStorage.cpp
)

add_custom_target(simulator_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main simulator)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)

	add_test(${TEST_NAME} ${TEST_NAME})
	add_dependencies(simulator_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Examples
##########################################################

set(EXAMPLE_SOURCES
	examples/basic.cpp
	examples/network.cpp
	examples/timer.cpp
	examples/transport.cpp
)

add_custom_target(simulator_examples)
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
	get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
	add_executable(simulator_${EXAMPLE_NAME}_example ${EXAMPLE_SOURCE})
	target_link_libraries(simulator_${EXAMPLE_NAME}_example PUBLIC simulator marlin::core)
	target_compile_options(simulator_${EXAMPLE_NAME}_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(simulator_${EXAMPLE_NAME}_example PRIVATE cxx_std_17)

	add_dependencies(simulator_examples simulator_${EXAMPLE_NAME}_example)
endforeach(EXAMPLE_SOURCE)
