cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(asyncio VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(asyncio INTERFACE)
add_library(marlin::asyncio ALIAS asyncio)

target_include_directories(asyncio
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(asyncio INTERFACE cxx_std_17)

# marlin::core
target_link_libraries(asyncio INTERFACE marlin::core)

# marlin::uvpp
target_link_libraries(asyncio INTERFACE marlin::uvpp)

# marlin::simulator
target_link_libraries(asyncio INTERFACE marlin::simulator)

# libuv
target_link_libraries(asyncio INTERFACE libuv::uv_a)

# spdlog
target_link_libraries(asyncio INTERFACE spdlog::spdlog_header_only)

install(TARGETS asyncio
	EXPORT marlin-asyncio-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-asyncio-export
	FILE
		MarlinAsyncioTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinAsyncio
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinAsyncioConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinAsyncio
)


##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES
	test/testUdp.cpp
)

add_custom_target(asyncio_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main asyncio)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(asyncio_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Examples
##########################################################

set(EXAMPLE_SOURCES
	examples/graph.cpp
	examples/udp.cpp
	examples/udp_fiber.cpp
	examples/tcp.cpp
	examples/tcp_out_fiber.cpp
	examples/timer.cpp
)

add_custom_target(asyncio_examples)
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
	get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
	add_executable(${EXAMPLE_NAME}_example ${EXAMPLE_SOURCE})
	target_link_libraries(${EXAMPLE_NAME}_example PUBLIC asyncio)
	target_compile_options(${EXAMPLE_NAME}_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${EXAMPLE_NAME}_example PRIVATE cxx_std_17)

	add_dependencies(asyncio_examples ${EXAMPLE_NAME}_example)
endforeach(EXAMPLE_SOURCE)


##########################################################
# All
##########################################################

add_custom_target(asyncio_all)
add_dependencies(asyncio_all asyncio asyncio_tests asyncio_examples)
