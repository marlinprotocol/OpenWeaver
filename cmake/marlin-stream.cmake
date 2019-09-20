find_package(MarlinStream)
if(NOT MarlinStream_FOUND)
	message("Using internal MarlinStream")
	include(FetchContent)
	FetchContent_Declare(MarlinStream
		GIT_REPOSITORY https://gitlab.com/marlinprotocol/stream.cpp.git
		GIT_TAG master
	)
	FetchContent_MakeAvailable(MarlinStream)

	add_library(Marlin::stream ALIAS stream)
endif()