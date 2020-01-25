cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(lpf VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(lpf INTERFACE)
add_library(marlin::lpf ALIAS lpf)

target_include_directories(lpf
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# marlin::stream
target_link_libraries(lpf INTERFACE marlin::stream)

# spdlog
target_link_libraries(lpf INTERFACE spdlog::spdlog)

install(TARGETS lpf
	EXPORT marlin-lpf-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-lpf-export
	FILE
		MarlinLpfTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinLpf
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinLpfConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinLpf
)

##########################################################
# Build examples
##########################################################

add_executable(lpf_example
	examples/lpf.cpp
)
target_link_libraries(lpf_example PUBLIC lpf)
