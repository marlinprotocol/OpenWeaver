find_package(snappy)
if(NOT snappy_FOUND)
	message("Using internal snappy")

	include(FetchContent)

	FetchContent_Declare(snappy
		GIT_REPOSITORY https://github.com/google/snappy.git
		GIT_TAG 1.1.7
	)
	FetchContent_MakeAvailable(snappy)

	add_library(Snappy::snappy ALIAS snappy)
endif()
