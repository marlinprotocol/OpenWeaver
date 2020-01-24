find_package(MarlinStream QUIET)
if(NOT MarlinStream_FOUND)
	message("-- MarlinStream not found. Using internal MarlinStream.")
    include(FetchContent)
    FetchContent_Declare(MarlinStream
        GIT_REPOSITORY https://gitlab.com/marlinprotocol/stream.cpp.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(MarlinStream)

    add_library(Marlin::stream ALIAS stream)
else()
	message("-- MarlinStream found. Using system MarlinStream.")
endif()
