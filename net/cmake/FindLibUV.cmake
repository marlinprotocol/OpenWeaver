FIND_PATH(LibUV_INCLUDE_DIR NAMES uv.h)
FIND_LIBRARY(LibUV_LIBRARY NAMES libuv.a libuv_a.a)

INCLUDE(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibUV
	REQUIRED_VARS LibUV_LIBRARY LibUV_INCLUDE_DIR
)

if(LibUV_FOUND AND NOT TARGET uv_a)
	add_library(uv_a STATIC IMPORTED)

	find_package(Threads)
	set_target_properties(uv_a PROPERTIES
		IMPORTED_LOCATION ${LibUV_LIBRARY}
		INTERFACE_LINK_LIBRARIES Threads::Threads
		INTERFACE_INCLUDE_DIRECTORIES "${LibUV_INCLUDE_DIR}"
	)
endif()
