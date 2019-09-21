find_package(Snappy QUIET)
if(NOT Snappy_FOUND)
	message("-- Snappy not found. Using internal Snappy.")

	include(FetchContent)

	FetchContent_Declare(Snappy
		GIT_REPOSITORY https://github.com/google/snappy.git
		GIT_TAG d837d5cfe1bf7b0eb52220428bc3541025db86cb
	)
	FetchContent_MakeAvailable(Snappy)

	add_library(Snappy::snappy ALIAS snappy)
else()
	message("-- Snappy found. Using system Snappy.")
endif()
