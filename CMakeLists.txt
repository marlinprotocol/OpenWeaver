cmake_minimum_required(VERSION 3.9)
project(onramp-eth VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_executable(onramp-eth
	src/onramp.cpp
)

target_include_directories(onramp-eth
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(onramp-eth PROPERTIES
	OUTPUT_NAME "onramp_eth"
)

find_package(MarlinPubsub CONFIG REQUIRED COMPONENTS pubsub
	NAMES "Marlin" CONFIGS "MarlinPubsubConfig.cmake")
target_link_libraries(onramp-eth PUBLIC Marlin::pubsub)

find_package(MarlinRlpx CONFIG REQUIRED COMPONENTS rlpx
	NAMES "Marlin" CONFIGS "MarlinRlpxConfig.cmake")
target_link_libraries(onramp-eth PUBLIC Marlin::rlpx)

find_package(MarlinBeacon CONFIG REQUIRED COMPONENTS beacon
	NAMES "Marlin" CONFIGS "MarlinBeaconConfig.cmake")
target_link_libraries(onramp-eth PUBLIC Marlin::beacon)

target_link_libraries(onramp-eth PUBLIC cryptopp)

find_package(spdlog REQUIRED)
