cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(abci_tm VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(abci_tm INTERFACE)
add_library(marlin::abci_tm ALIAS abci_tm)

target_include_directories(abci_tm
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(abci_tm INTERFACE cxx_std_17)

# marlin::asyncio
target_link_libraries(abci_tm INTERFACE marlin::asyncio)

# marlin::simulator
target_link_libraries(abci_tm INTERFACE marlin::simulator)

# spdlog
target_link_libraries(abci_tm INTERFACE spdlog::spdlog_header_only)

# structopt
target_link_libraries(abci_tm INTERFACE structopt::structopt)

# rapidjson
target_link_libraries(abci_tm INTERFACE rapidjson)

# cryptopp
target_link_libraries(abci_tm INTERFACE cryptopp::CryptoPP)

# secp256k1
target_link_libraries(abci_tm INTERFACE secp256k1::secp256k1)

# boost::filesystem
target_link_libraries(abci_tm INTERFACE Boost::filesystem)

install(TARGETS abci_tm
	EXPORT marlin-abci-tm-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-abci-tm-export
	FILE
		MarlinAbcitmTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinAbcitm
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinAbcitmConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinAbcitm
)


##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES
)

add_custom_target(abci_tm_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main abci_tm)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(abci_tm_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Examples
##########################################################

set(EXAMPLE_SOURCES
	examples/abci.cpp
)

add_custom_target(abci_tm_examples)
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
	get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
	add_executable(tm_${EXAMPLE_NAME}_example ${EXAMPLE_SOURCE})
	target_link_libraries(tm_${EXAMPLE_NAME}_example PUBLIC abci_tm)
	target_compile_options(tm_${EXAMPLE_NAME}_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(tm_${EXAMPLE_NAME}_example PRIVATE cxx_std_17)

	add_dependencies(abci_tm_examples tm_${EXAMPLE_NAME}_example)
endforeach(EXAMPLE_SOURCE)


##########################################################
# All
##########################################################

add_custom_target(abci_tm_all)
add_dependencies(abci_tm_all abci_tm abci_tm_tests abci_tm_examples)
