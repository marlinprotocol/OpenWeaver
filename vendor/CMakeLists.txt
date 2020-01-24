#-------- libuv --------#
add_subdirectory(libuv)

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
