find_package(libsodium QUIET)
if(NOT libsodium_FOUND)
	message("-- libsodium not found. Using internal libsodium.")

	include(FetchContent)

	FetchContent_Declare(libsodium
		GIT_REPOSITORY https://github.com/jedisct1/libsodium.git
		GIT_TAG 1.0.18
	)
	FetchContent_MakeAvailable(libsodium)
else()
	message("-- libsodium found. Using system libsodium.")
endif()
