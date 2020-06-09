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

add_library(mtest INTERFACE)
add_library(marlin::mtest ALIAS mtest)

target_include_directories(mtest
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(mtest INTERFACE cxx_std_17)

# marlin::simulator
target_link_libraries(mtest INTERFACE marlin::simulator)

# spdlog
target_link_libraries(mtest INTERFACE spdlog::spdlog)

# gtest
target_link_libraries(mtest INTERFACE GTest::GTest)

install(TARGETS mtest
	EXPORT marlin-mtest-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-mtest-export
	FILE
		MarlinMtestTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinMtest
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinMtestConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinMtest
)


##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES

)

add_custom_target(mtest_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main mtest marlin::simulator)
	target_compile_definitions(${TEST_NAME} PRIVATE MARLIN_ASYNCIO_SIMULATOR)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(mtest_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_custom_target(mtest_examples)


##########################################################
# All
##########################################################

add_custom_target(mtest_all)
add_dependencies(mtest_all mtest mtest_tests mtest_examples)
