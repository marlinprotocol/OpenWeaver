cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(abci_eth VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(abci_eth INTERFACE)
add_library(marlin::abci_eth ALIAS abci_eth)

target_include_directories(abci_eth
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(abci_eth INTERFACE cxx_std_17)

# marlin::asyncio
target_link_libraries(abci_eth INTERFACE marlin::asyncio)

# marlin::simulator
target_link_libraries(abci_eth INTERFACE marlin::simulator)

# spdlog
target_link_libraries(abci_eth INTERFACE spdlog::spdlog_header_only)

# structopt
target_link_libraries(abci_eth INTERFACE structopt::structopt)

# rapidjson
target_link_libraries(abci_eth INTERFACE rapidjson)

# cryptopp
target_link_libraries(abci_eth INTERFACE cryptopp::CryptoPP)

# secp256k1
target_link_libraries(abci_eth INTERFACE secp256k1::secp256k1)

# boost::filesystem
target_link_libraries(abci_eth INTERFACE Boost::filesystem)

install(TARGETS abci_eth
	EXPORT marlin-abci-eth-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-abci-eth-export
	FILE
		MarlinAbciEthTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinAbciEth
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinAbciEthConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinAbciEth
)


##########################################################
# Tests
##########################################################

enable_testing()

set(TEST_SOURCES
)

add_custom_target(abci_eth_tests)
foreach(TEST_SOURCE ${TEST_SOURCES})
	get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
	add_executable(${TEST_NAME} ${TEST_SOURCE})
	target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main abci_eth)
	target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
	add_test(${TEST_NAME} ${TEST_NAME})

	add_dependencies(abci_eth_tests ${TEST_NAME})
endforeach(TEST_SOURCE)


##########################################################
# Examples
##########################################################

set(EXAMPLE_SOURCES
	examples/abci.cpp
)

add_custom_target(abci_eth_examples)
foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
	get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
	add_executable(eth_${EXAMPLE_NAME}_example ${EXAMPLE_SOURCE})
	target_link_libraries(eth_${EXAMPLE_NAME}_example PUBLIC abci_eth)
	target_compile_options(eth_${EXAMPLE_NAME}_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
	target_compile_features(eth_${EXAMPLE_NAME}_example PRIVATE cxx_std_17)

	add_dependencies(abci_eth_examples eth_${EXAMPLE_NAME}_example)
endforeach(EXAMPLE_SOURCE)


##########################################################
# All
##########################################################

add_custom_target(abci_eth_all)
add_dependencies(abci_eth_all abci_eth abci_eth_tests abci_eth_examples)
