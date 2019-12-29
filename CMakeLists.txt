cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(multicastsdk VERSION 0.0.1 LANGUAGES C CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(multicastsdk INTERFACE)

target_include_directories(multicastsdk
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# MarlinPubsub
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-pubsub.cmake")
target_link_libraries(multicastsdk INTERFACE Marlin::pubsub)

# MarlinBeacon
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-beacon.cmake")
target_link_libraries(multicastsdk INTERFACE Marlin::beacon)

# MarlinNet
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-net.cmake")
target_link_libraries(multicastsdk INTERFACE Marlin::net)

# Spdlog
include("${CMAKE_CURRENT_LIST_DIR}/cmake/spdlog.cmake")
target_link_libraries(multicastsdk INTERFACE spdlog::spdlog)

install(TARGETS multicastsdk
	EXPORT marlin-multicastsdk-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-multicastsdk-export
  FILE
	MarlinMulticastSDKTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/MarlinMulticastSDK
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinMulticastSDKConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MarlinMulticastSDK
)


##########################################################
# C wrapper examples
##########################################################

add_library(multicastsdk_c STATIC
	src/MarlinMulticastClient.cpp
)

target_include_directories(multicastsdk_c
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(multicastsdk_c INTERFACE spdlog::spdlog)
target_link_libraries(multicastsdk_c PUBLIC multicastsdk)

target_compile_options(multicastsdk_c PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(multicastsdk_c PUBLIC cxx_std_17)

# add_executable(multicastsdk_ctest
# 	src/multicastsdk_ctest.c
# )

# target_link_libraries(multicastsdk_ctest PUBLIC multicastsdk_c)
# target_compile_features(multicastsdk_ctest PUBLIC c_std_11)
