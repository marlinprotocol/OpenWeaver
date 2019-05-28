cmake_minimum_required(VERSION 3.9)
project(stream VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(stream INTERFACE)

target_include_directories(stream
	INTERFACE
		$<INSTALL_INTERFACE:include/marlin/stream>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# set_target_properties(stream PROPERTIES
# 	OUTPUT_NAME "marlin-stream"
# )

find_package(MarlinNet CONFIG REQUIRED COMPONENTS net
	NAMES "Marlin" CONFIGS "MarlinNetConfig.cmake")

target_link_libraries(stream INTERFACE Marlin::net)

find_package(spdlog REQUIRED)

install(TARGETS stream
	EXPORT marlin-stream-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/stream)

##########################################################
# Export
##########################################################

install(EXPORT marlin-stream-export
  FILE
	MarlinStreamTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinStreamConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)


##########################################################
# Tests
##########################################################

enable_testing()
find_package(GTest REQUIRED)

set(TEST_SOURCES
	test/testStreamProtocol.cpp
	test/testAckRanges.cpp
)

foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main Marlin::net stream)
	target_include_directories(${TEST_NAME}
		PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/include
	)
	add_test(${TEST_NAME} ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_executable(testnode
	examples/main.cpp
)

target_include_directories(testnode
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)

target_link_libraries(testnode PUBLIC stream)

add_executable(testclient
	examples/client.cpp
)

target_include_directories(testclient
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)

target_link_libraries(testclient PUBLIC stream)
