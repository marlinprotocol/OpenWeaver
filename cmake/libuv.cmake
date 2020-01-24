find_package(LibUV QUIET)
if(NOT LibUV_FOUND)
	message("-- LibUV not found. Using internal LibUV.")

	include(FetchContent)

	find_package(Threads REQUIRED)
	FetchContent_Declare(libuv
		GIT_REPOSITORY https://github.com/libuv/libuv.git
		GIT_TAG v1.32.0
	)
	FetchContent_MakeAvailable(libuv)

	# Avert your eyes
	set_property(TARGET uv_a PROPERTY INTERFACE_INCLUDE_DIRECTORIES
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${libuv_SOURCE_DIR}/include>
	)
	get_directory_property(uv_libraries_fixed
		DIRECTORY ${libuv_SOURCE_DIR}
		DEFINITION uv_libraries
	)
	list(APPEND uv_libraries_fixed Threads::Threads)
	set_property(TARGET uv_a PROPERTY INTERFACE_LINK_LIBRARIES
		${uv_libraries_fixed}
	)

	install(TARGETS uv_a
		EXPORT marlin-net-export
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
else()
	message("-- LibUV found. Using system LibUV.")
endif()
