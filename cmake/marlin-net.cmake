find_package(MarlinNet QUIET)
if(NOT MarlinNet_FOUND)
	message("-- MarlinNet not found. Using internal MarlinNet.")
    include(FetchContent)
    FetchContent_Declare(MarlinNet
        GIT_REPOSITORY https://gitlab.com/marlinprotocol/net.cpp.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(MarlinNet)

    add_library(Marlin::net ALIAS net)
else()
	message("-- MarlinNet found. Using system MarlinNet.")
endif()
