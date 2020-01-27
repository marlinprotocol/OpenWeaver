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

add_executable(relayexec
	src/relay_exec.cpp
)

add_executable(masterexec
	src/master_exec.cpp
)

target_include_directories(relayexec
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(masterexec
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

IF(APPLE)
target_link_libraries(relayexec PUBLIC c++)
target_link_libraries(masterexec PUBLIC c++)

ELSE()
target_link_libraries(relayexec PUBLIC stdc++fs)
target_link_libraries(masterexec PUBLIC stdc++fs)
ENDIF()

target_compile_options(relayexec PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(relayexec PUBLIC cxx_std_17)
target_compile_options(masterexec PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(masterexec PUBLIC cxx_std_17)

# marlin::pubsub
target_link_libraries(relayexec PUBLIC marlin::pubsub)
target_link_libraries(masterexec PUBLIC marlin::pubsub)

# marlin::beacon
target_link_libraries(relayexec PUBLIC marlin::beacon)
target_link_libraries(masterexec PUBLIC marlin::beacon)
