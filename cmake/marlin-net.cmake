find_package(MarlinNet)
if(NOT MarlinNet_FOUND)
    message("Using internal MarlinNet")
    include(FetchContent)
    FetchContent_Declare(MarlinNet
        GIT_REPOSITORY https://gitlab.com/marlinprotocol/net.cpp.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(MarlinNet)
endif()
