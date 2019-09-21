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
	src/CidrBlock.cpp
	src/Buffer.cpp
	src/SocketAddress.cpp
)

target_include_directories(net
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_options(net PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(net PRIVATE cxx_std_17)

set_target_properties(net PROPERTIES
	OUTPUT_NAME "marlin-net"
)

# LibUV
include("${CMAKE_CURRENT_LIST_DIR}/cmake/libuv.cmake")
target_link_libraries(net PUBLIC uv_a)

# Spdlog
include("${CMAKE_CURRENT_LIST_DIR}/cmake/spdlog.cmake")
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
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/MarlinNet
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/FindLibUV.cmake
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinNetConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MarlinNet
)


##########################################################
# Tests
##########################################################

enable_testing()
# GTest
include("${CMAKE_CURRENT_LIST_DIR}/cmake/gtest.cmake")

set(TEST_SOURCES
	test/testBuffer.cpp
	test/testEndian.cpp
	test/testSocketAddress.cpp
	test/testUdp.cpp
)

foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main net)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
	add_test(${TEST_NAME} ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Examples
##########################################################

add_executable(udp_example
	examples/udp.cpp
)
target_link_libraries(udp_example PUBLIC net)
target_compile_options(udp_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(udp_example PRIVATE cxx_std_17)

add_executable(tcp_example
	examples/tcp.cpp
)
target_link_libraries(tcp_example PUBLIC net)
target_compile_options(tcp_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(tcp_example PRIVATE cxx_std_17)
