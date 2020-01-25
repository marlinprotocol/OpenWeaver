cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(pubsub VERSION 0.0.1 LANGUAGES CXX)

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

add_library(pubsub INTERFACE)
add_library(marlin::pubsub ALIAS pubsub)

target_include_directories(pubsub
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(pubsub INTERFACE cxx_std_17)

# marlin::lpf
target_link_libraries(pubsub INTERFACE marlin::lpf)

# marlin::stream
target_link_libraries(pubsub INTERFACE marlin::stream)

# marlin::net
target_link_libraries(pubsub INTERFACE marlin::net)

# spdlog
target_link_libraries(pubsub INTERFACE spdlog::spdlog)

install(TARGETS pubsub
	EXPORT marlin-pubsub-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-pubsub-export
	FILE
		MarlinPubsubTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinPubsub
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinPubsubConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinPubsub
)

##########################################################
# Build examples
##########################################################

add_executable(testclient
	examples/client.cpp
)

target_link_libraries(testclient PUBLIC pubsub)
target_compile_options(testclient PRIVATE -Werror -Wall -Wextra -pedantic-errors -ftemplate-backtrace-limit=0)
