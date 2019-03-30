cmake_minimum_required(VERSION 3.9)
project(stream VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# add_library(stream STATIC

# )

# target_include_directories(stream
# 	PUBLIC
# 		$<INSTALL_INTERFACE:include/marlin/stream>
# 		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
# 	PRIVATE
# 		${CMAKE_CURRENT_SOURCE_DIR}/src
# )

# set_target_properties(stream PROPERTIES
# 	OUTPUT_NAME "marlin-stream"
# )

# find_package(MarlinTest CONFIG REQUIRED COMPONENTS test
# 	NAMES "Marlin" CONFIGS "MarlinTestConfig.cmake")

# target_link_libraries(stream PUBLIC Marlin::test)

# find_package(spdlog REQUIRED)

# install(TARGETS stream
# 	EXPORT marlin-stream-export
# 	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
# 	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
# )

# install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/stream)

##########################################################
# Export
##########################################################

# install(EXPORT marlin-stream-export
#   FILE
# 	MarlinStreamTargets.cmake
#   NAMESPACE
# 	Marlin::
#   DESTINATION
# 	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
# )

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinStreamConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

##########################################################
# Build examples
##########################################################

add_executable(testnode EXCLUDE_FROM_ALL
	examples/main.cpp
)

target_include_directories(testnode
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/examples
)

find_package(MarlinNet CONFIG REQUIRED COMPONENTS net
	NAMES "Marlin" CONFIGS "MarlinNetConfig.cmake")

target_link_libraries(testnode PUBLIC Marlin::net)
