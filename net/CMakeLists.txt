cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(net VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(net STATIC
	src/core/BN.cpp
	src/CidrBlock.cpp
	src/Buffer.cpp
	src/SocketAddress.cpp
)
add_library(marlin::net ALIAS net)

target_include_directories(net
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_options(net PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(net PUBLIC cxx_std_17)

set_target_properties(net PROPERTIES
	OUTPUT_NAME "marlin-net"
)

# libuv
target_link_libraries(net PUBLIC libuv::uv_a)

# spdlog
target_link_libraries(net PUBLIC spdlog::spdlog)

install(TARGETS net
	EXPORT marlin-net-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-net-export
	FILE
		MarlinNetTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinNet
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinNetConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinNet
)


##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES
	test/testBN.cpp
	test/testBuffer.cpp
	test/testEndian.cpp
	test/testSocketAddress.cpp
	test/testUdp.cpp
)

add_custom_target(net_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main net)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(net_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Examples
##########################################################

set(EXAMPLE_SOURCES
	examples/udp.cpp
	examples/tcp.cpp
	examples/timer.cpp
)

add_custom_target(net_examples)
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
	get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
	add_executable(${EXAMPLE_NAME}_example ${EXAMPLE_SOURCE})
	target_link_libraries(${EXAMPLE_NAME}_example PUBLIC net)
	target_compile_options(${EXAMPLE_NAME}_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${EXAMPLE_NAME}_example PRIVATE cxx_std_17)

	add_dependencies(net_examples ${EXAMPLE_NAME}_example)
endforeach(EXAMPLE_SOURCE)


##########################################################
# All
##########################################################

add_custom_target(net_all)
add_dependencies(net_all net net_tests net_examples)
