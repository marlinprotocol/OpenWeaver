cmake_minimum_required(VERSION 3.9)
project(lpf VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(lpf INTERFACE)

target_include_directories(lpf
	INTERFACE
		$<INSTALL_INTERFACE:include/marlin/lpf>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# set_target_properties(lpf PROPERTIES
# 	OUTPUT_NAME "marlin-lpf"
# )

find_package(MarlinNet CONFIG REQUIRED COMPONENTS net
	NAMES "Marlin" CONFIGS "MarlinNetConfig.cmake")
target_link_libraries(lpf INTERFACE Marlin::net)

find_package(spdlog REQUIRED)

install(TARGETS lpf
	EXPORT marlin-lpf-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/lpf)

##########################################################
# Export
##########################################################

install(EXPORT marlin-lpf-export
  FILE
	MarlinLpfTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinLpfConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

##########################################################
# Build examples
##########################################################

add_executable(lpf_example
	examples/lpf.cpp
)

target_include_directories(lpf_example
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)

target_link_libraries(lpf_example PUBLIC lpf)
