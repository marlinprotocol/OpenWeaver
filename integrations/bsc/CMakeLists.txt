cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(onramp-bsc VERSION 0.0.1 LANGUAGES CXX)

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

add_executable(onramp-bsc
	src/onramp.cpp
)

target_include_directories(onramp-bsc
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_options(onramp-bsc PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(onramp-bsc PUBLIC cxx_std_17)

set_target_properties(onramp-bsc PROPERTIES
	OUTPUT_NAME "onramp_bsc"
)

# marlin::multicastsdk
target_link_libraries(onramp-bsc PUBLIC marlin::multicastsdk)

# marlin::rlpx
target_link_libraries(onramp-bsc PUBLIC marlin::rlpx)

# spdlog
target_link_libraries(onramp-bsc PUBLIC spdlog::spdlog_header_only)
