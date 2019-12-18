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

add_executable(genericRelay
	src/main.cpp
)

target_include_directories(genericRelay
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

IF(APPLE)
target_link_libraries(genericRelay PUBLIC c++)
ELSE()
target_link_libraries(genericRelay PUBLIC stdc++fs)
ENDIF()

target_compile_options(genericRelay PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(genericRelay PUBLIC cxx_std_17)

# MarlinPubsub
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-pubsub.cmake")
target_link_libraries(genericRelay PUBLIC Marlin::pubsub)

# MarlinBeacon
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-beacon.cmake")
target_link_libraries(genericRelay PUBLIC Marlin::beacon)

# spdlog
include("${CMAKE_CURRENT_LIST_DIR}/cmake/spdlog.cmake")
target_link_libraries(genericRelay PUBLIC spdlog::spdlog)
