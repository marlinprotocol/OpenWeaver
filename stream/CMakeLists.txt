cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(stream VERSION 0.0.1 LANGUAGES CXX)

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

add_library(stream INTERFACE)
add_library(marlin::stream ALIAS stream)

target_include_directories(stream
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(stream INTERFACE cxx_std_17)

# libsodium
target_link_libraries(stream INTERFACE sodium::sodium)

# marlin::asyncio
target_link_libraries(stream INTERFACE marlin::asyncio)

# spdlog
target_link_libraries(stream INTERFACE spdlog::spdlog_header_only)

# Boost::filesystem
target_link_libraries(stream INTERFACE Boost::filesystem)

install(TARGETS stream
	EXPORT marlin-stream-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-stream-export
	FILE
		MarlinStreamTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinStream
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinStreamConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinStream
)

##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES
	test/testAckRanges.cpp
)

add_custom_target(stream_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main stream)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(stream_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_custom_target(stream_examples)

add_executable(stream_example
	examples/stream.cpp
)
add_dependencies(stream_examples stream_example)

target_link_libraries(stream_example PUBLIC stream)
target_compile_options(stream_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(stream_example PRIVATE cxx_std_17)

add_executable(stream_server
	examples/server.cpp
)
add_dependencies(stream_examples stream_server)

IF(APPLE)
target_link_libraries(stream_server PUBLIC stream c++)
ELSE()
target_link_libraries(stream_server PUBLIC stream stdc++fs)
ENDIF()

target_compile_options(stream_server PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(stream_server PRIVATE cxx_std_17)

add_executable(stream_client
	examples/client.cpp
)
add_dependencies(stream_examples stream_client)

IF(APPLE)
target_link_libraries(stream_client PUBLIC stream c++)
ELSE()
target_link_libraries(stream_client PUBLIC stream stdc++fs)
ENDIF()

target_compile_options(stream_client PRIVATE -Werror -Wall -Wimplicit-fallthrough -Wextra -pedantic-errors)
target_compile_features(stream_client PRIVATE cxx_std_17)

add_executable(stream_simulated_example
	examples/stream_simulated.cpp
)
add_dependencies(stream_examples stream_simulated_example)

target_link_libraries(stream_simulated_example PUBLIC stream marlin::simulator)
target_compile_definitions(${TEST_NAME} PRIVATE MARLIN_ASYNCIO_SIMULATOR)
target_compile_options(stream_simulated_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(stream_simulated_example PRIVATE cxx_std_17)


##########################################################
# All
##########################################################

add_custom_target(stream_all)
add_dependencies(stream_all stream stream_tests stream_examples)
