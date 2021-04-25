cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(relay VERSION 0.0.1 LANGUAGES CXX)

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

add_library(relay INTERFACE)
target_include_directories(relay
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(relay INTERFACE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(relay INTERFACE cxx_std_17)

# marlin::pubsub
target_link_libraries(relay INTERFACE marlin::pubsub)

# marlin::beacon
target_link_libraries(relay INTERFACE marlin::beacon)

# boost::filesystem
target_link_libraries(relay INTERFACE Boost::filesystem)

add_executable(relayexec
	src/relay_exec.cpp
)
add_executable(masterexec
	src/master_exec.cpp
)

target_link_libraries(relayexec PUBLIC relay)
target_link_libraries(masterexec PUBLIC relay)

add_executable(bsc_relay
	src/bsc_relay.cpp
)
target_link_libraries(bsc_relay PUBLIC relay marlin::abci_bsc)

add_executable(eth_relay
	src/bsc_relay.cpp
)
target_link_libraries(eth_relay PUBLIC relay marlin::abci_bsc)


function(relay_exe EXE_PREFIX DEFAULT_PUBSUB DEFAULT_DISC)
	set(EXE_NAME ${EXE_PREFIX}_relay)
	add_executable(${EXE_NAME}
		src/relay.cpp
	)

	target_link_libraries(${EXE_NAME} PUBLIC relay)

	# structopt
	target_link_libraries(${EXE_NAME} PUBLIC structopt::structopt)

	target_compile_definitions(${EXE_NAME} PUBLIC
		MARLIN_RELAY_DEFAULT_PUBSUB_PORT=${DEFAULT_PUBSUB}
		MARLIN_RELAY_DEFAULT_DISC_PORT=${DEFAULT_DISC}
	)
endfunction()

relay_exe(dot 20500 20502)
relay_exe(iris 21500 21502)
relay_exe(cosmos 22000 22002)

