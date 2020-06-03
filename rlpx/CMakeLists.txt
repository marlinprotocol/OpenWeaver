cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(rlpx VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

IF(APPLE)
    # Fix linking on 10.14+. See https://stackoverflow.com/questions/54068035
    LINK_DIRECTORIES(/usr/local/lib)
ENDIF()

add_library(rlpx STATIC
	src/RlpxCrypto.cpp
)

target_include_directories(rlpx
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_options(rlpx PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(rlpx PUBLIC cxx_std_17)

set_target_properties(rlpx PROPERTIES
	OUTPUT_NAME "marlin-rlpx"
)

# Cryptopp
include("${CMAKE_CURRENT_LIST_DIR}/cmake/cryptopp.cmake")
target_link_libraries(rlpx PUBLIC cryptopp)

# MarlinNet
include("${CMAKE_CURRENT_LIST_DIR}/cmake/marlin-net.cmake")
target_link_libraries(rlpx PUBLIC Marlin::net)

# spdlog
include("${CMAKE_CURRENT_LIST_DIR}/cmake/spdlog.cmake")
target_link_libraries(rlpx PUBLIC spdlog::spdlog)

# snappy
include("${CMAKE_CURRENT_LIST_DIR}/cmake/snappy.cmake")
target_link_libraries(rlpx PUBLIC Snappy::snappy)

install(TARGETS rlpx
	EXPORT marlin-rlpx-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-rlpx-export
  FILE
	MarlinRlpxTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/MarlinRlpx
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinRlpxConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MarlinRlpx
)

##########################################################
# Build examples
##########################################################

add_executable(rlpx_example
	examples/rlpx.cpp
)
target_link_libraries(rlpx_example PUBLIC rlpx)
target_compile_options(rlpx_example PRIVATE -Werror -Wall -Wextra -pedantic-errors)
