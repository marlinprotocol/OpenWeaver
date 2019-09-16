cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(net VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# LibUV
find_package(LibUV)
if(NOT LibUV_FOUND)
	message("Using internal libuv")

	include(FetchContent)

	find_package(Threads REQUIRED)
	FetchContent_Declare(libuv
		GIT_REPOSITORY https://github.com/libuv/libuv.git
		GIT_TAG v1.32.0
	)
	FetchContent_MakeAvailable(libuv)

	# Avert your eyes
	set_property(TARGET uv_a PROPERTY INTERFACE_INCLUDE_DIRECTORIES
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${libuv_SOURCE_DIR}/include>
	)
	get_directory_property(uv_libraries_fixed
		DIRECTORY ${libuv_SOURCE_DIR}
		DEFINITION uv_libraries
	)
	list(APPEND uv_libraries_fixed Threads::Threads)
	set_property(TARGET uv_a PROPERTY INTERFACE_LINK_LIBRARIES
		${uv_libraries_fixed}
	)

	install(TARGETS uv_a
		EXPORT marlin-net-export
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
endif()

# Spdlog
find_package(spdlog)
if(NOT spdlog_FOUND)
	message("Using internal spdlog")

	include(FetchContent)

	FetchContent_Declare(spdlog
		GIT_REPOSITORY https://github.com/gabime/spdlog.git
		GIT_TAG v1.3.1
	)
	FetchContent_MakeAvailable(spdlog)

	install(TARGETS spdlog
		EXPORT marlin-net-export
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
endif()

add_library(net STATIC
	src/CidrBlock.cpp
	src/Buffer.cpp
	src/SocketAddress.cpp
)

target_include_directories(net
	PUBLIC
		$<INSTALL_INTERFACE:include/marlin/net>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(net PROPERTIES
	OUTPUT_NAME "marlin-net"
)

target_link_libraries(net PUBLIC uv_a spdlog::spdlog)

install(TARGETS net
	EXPORT marlin-net-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/net)

##########################################################
# Export
##########################################################

install(EXPORT marlin-net-export
  FILE
	MarlinNetTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/FindLibUV.cmake
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinNetConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)


##########################################################
# Tests
##########################################################

enable_testing()
find_package(GTest REQUIRED)

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
	add_test(${TEST_NAME} ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Examples
##########################################################

add_executable(udp_example
	examples/udp.cpp
)

target_include_directories(udp_example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(udp_example PUBLIC net)

add_executable(tcp_example
	examples/tcp.cpp
)

target_include_directories(tcp_example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(tcp_example PUBLIC net)
