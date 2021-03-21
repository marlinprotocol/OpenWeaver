cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(uvpp VERSION 0.0.1 LANGUAGES CXX)

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

add_library(uvpp INTERFACE)
add_library(marlin::uvpp ALIAS uvpp)

target_include_directories(uvpp
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(uvpp INTERFACE cxx_std_17)

# libuv
target_link_libraries(uvpp INTERFACE libuv::uv_a)

# marlin::core
target_link_libraries(uvpp INTERFACE marlin::core)

install(TARGETS uvpp
	EXPORT marlin-uvpp-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-uvpp-export
	FILE
		MarlinUvppTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinUvpp
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinUvppConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinUvpp
)

##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES
)

add_custom_target(uvpp_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main uvpp)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(uvpp_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_custom_target(uvpp_examples)


##########################################################
# All
##########################################################

add_custom_target(uvpp_all)
add_dependencies(uvpp_all uvpp uvpp_tests uvpp_examples)
