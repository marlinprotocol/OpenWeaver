#-------- libuv --------#
add_subdirectory(libuv)
add_library(libuv::uv_a ALIAS uv_a)

# Avert your eyes
set_property(TARGET uv_a PROPERTY INTERFACE_INCLUDE_DIRECTORIES
	$<INSTALL_INTERFACE:include>
	$<BUILD_INTERFACE:${libuv_SOURCE_DIR}/include>
)

find_package(Threads REQUIRED COMPONENTS Threads)
target_link_libraries(uv_a Threads::Threads)

install(TARGETS uv_a
	EXPORT libuv-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT libuv-export
	FILE
		LibuvTargets.cmake
	NAMESPACE
		libuv::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/libuv
)


#-------- spdlog --------#
set(SPDLOG_MASTER_PROJECT ON)
add_subdirectory(spdlog)


#-------- libsodium --------#
include(ExternalProject)
ExternalProject_Add(libsodium
	SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libsodium
	CONFIGURE_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/libsodium && ./autogen.sh && ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/libsodium
	BUILD_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/libsodium && make -j8
	INSTALL_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/libsodium && make install
)

add_library(sodium STATIC IMPORTED GLOBAL)
add_library(sodium::sodium ALIAS sodium)
add_dependencies(sodium libsodium)

set_target_properties(sodium PROPERTIES
	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libsodium/lib/libsodium.a
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/libsodium/include
)
