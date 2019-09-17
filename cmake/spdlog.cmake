find_package(spdlog)
if(NOT spdlog_FOUND)
	message("Using internal spdlog")

	include(FetchContent)

	FetchContent_Declare(spdlog
		GIT_REPOSITORY https://github.com/gabime/spdlog.git
		GIT_TAG v1.3.1
	)
	FetchContent_MakeAvailable(spdlog)

	install(TARGETS spdlog
		EXPORT marlin-net-export
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	)
endif()
