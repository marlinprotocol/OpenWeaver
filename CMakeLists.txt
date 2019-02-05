cmake_minimum_required(VERSION 3.9)
project(stream VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# add_library(weave SHARED
# 	src/node/Beacon.cpp
# 	src/node/Node.cpp
# )

# target_include_directories(weave
# 	PUBLIC
# 		$<INSTALL_INTERFACE:include>/node
# 		$<INSTALL_INTERFACE:include>/protocol
# 		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>/node
# 		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>/protocol
# 	PRIVATE
# 		${CMAKE_CURRENT_SOURCE_DIR}/src
# )

# set_target_properties(weave PROPERTIES
# 	OUTPUT_NAME "marlin-weave"
# )

# find_package(MarlinFiber CONFIG REQUIRED COMPONENTS fiber
# 	NAMES "Marlin" CONFIGS "MarlinFiberConfig.cmake")

# target_link_libraries(weave PUBLIC Marlin::fiber)

# install(TARGETS weave
# 	EXPORT marlin-weave-export
# 	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
# )

# install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/weave)

##########################################################
# Export
##########################################################

# install(EXPORT marlin-weave-export
#   FILE
# 	MarlinWeaveTargets.cmake
#   NAMESPACE
# 	Marlin::
#   DESTINATION
# 	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
# )

# install(FILES
#     ${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinWeaveConfig.cmake
#     DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
# )

##########################################################
# Build examples
##########################################################

add_executable(testnode EXCLUDE_FROM_ALL
	examples/main.cpp
)

find_package(MarlinWeave CONFIG REQUIRED COMPONENTS weave
	NAMES "Marlin" CONFIGS "MarlinWeaveConfig.cmake")

target_link_libraries(testnode PUBLIC Marlin::weave)
