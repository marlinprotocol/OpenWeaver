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
add_library(marlin::multicastsdk ALIAS multicastsdk)

target_include_directories(multicastsdk
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# marlin::pubsub
target_link_libraries(multicastsdk INTERFACE marlin::pubsub)

# marlin::beacon
target_link_libraries(multicastsdk INTERFACE marlin::beacon)

# marlin::net
target_link_libraries(multicastsdk INTERFACE marlin::net)

# spdlog
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
	marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/marlinMulticastSDK
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinMulticastSDKConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinMulticastSDK
)


##########################################################
# C++ SDK examples
##########################################################

add_executable(msggen
	examples/msggen.cpp
)

target_link_libraries(msggen PUBLIC multicastsdk)

add_executable(multicastTcpServer
	examples/MulticastTcpServer.cpp
)

target_link_libraries(multicastTcpServer PUBLIC multicastsdk)


# MarlinLpf
target_link_libraries(multicastTcpServer PUBLIC marlin::lpf)

# MarlinStream
target_link_libraries(multicastTcpServer PUBLIC marlin::stream)


##########################################################
# C SDK
##########################################################

add_library(multicastsdk_c SHARED
	src/MarlinMulticastClient.cpp
)
add_library(marlin::multicastsdk_c ALIAS multicastsdk_c)

target_include_directories(multicastsdk_c
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(multicastsdk_c PUBLIC spdlog::spdlog)
target_link_libraries(multicastsdk_c PUBLIC multicastsdk)

target_compile_options(multicastsdk_c PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(multicastsdk_c PUBLIC cxx_std_17)


##########################################################
# C SDK examples
##########################################################

add_executable(multicastsdk_c_example
	examples/main.c
)

target_link_libraries(multicastsdk_c_example PUBLIC multicastsdk_c)
target_compile_features(multicastsdk_c_example PUBLIC c_std_11)
