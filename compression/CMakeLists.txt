cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(compression VERSION 0.0.1 LANGUAGES CXX)

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

add_library(compression INTERFACE)
add_library(marlin::compression ALIAS compression)

target_include_directories(compression
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(compression INTERFACE cxx_std_17)

# marlin::core
target_link_libraries(compression INTERFACE marlin::core)

# spdlog
target_link_libraries(compression INTERFACE spdlog::spdlog_header_only)

# cryptopp
target_link_libraries(compression INTERFACE cryptopp::CryptoPP)

install(TARGETS compression
	EXPORT marlin-compression-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-compression-export
	FILE
		MarlinCompressionTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinCompression
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinCompressionConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinCompression
)


##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES

)

add_custom_target(compression_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main compression marlin::simulator)
	target_compile_definitions(${TEST_NAME} PRIVATE MARLIN_ASYNCIO_SIMULATOR)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(compression_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Build examples
##########################################################

add_custom_target(compression_examples)

add_executable(compressor_example
	examples/compressor.cpp
)
add_dependencies(compression_examples compressor_example)

target_include_directories(compressor_example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)
target_link_libraries(compressor_example PUBLIC compression)
target_compile_options(compressor_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)


##########################################################
# All
##########################################################

add_custom_target(compression_all)
add_dependencies(compression_all compression compression_tests compression_examples)
