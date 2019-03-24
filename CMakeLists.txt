cmake_minimum_required(VERSION 3.9)
project(net VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra -pedantic-errors")

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(net STATIC
	src/CidrBlock.cpp
	src/Packet.cpp
	src/Socket.cpp
	src/SocketAddress.cpp
)

target_include_directories(net
	PUBLIC
		$<INSTALL_INTERFACE:include/marlin/net>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(net PROPERTIES
	OUTPUT_NAME "marlin-net"
)

find_package(LibUV REQUIRED COMPONENTS uv)
target_link_libraries(net PUBLIC LibUV::uv)

find_package(spdlog REQUIRED)

install(TARGETS net
	EXPORT marlin-net-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/marlin/net)

##########################################################
# Export
##########################################################

install(EXPORT marlin-net-export
  FILE
	MarlinNetTargets.cmake
  NAMESPACE
	Marlin::
  DESTINATION
	${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)

install(FILES
    ${CMAKE_CURRENT_LIST_DIR}/cmake/FindLibUV.cmake
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinNetConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Marlin
)
