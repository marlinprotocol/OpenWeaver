find_package(MarlinRlpx)
if(NOT MarlinRlpx_FOUND)
    message("Using internal MarlinRlpx")
    include(FetchContent)
    FetchContent_Declare(MarlinRlpx
        GIT_REPOSITORY https://gitlab.com/marlinprotocol/rlpx.cpp.git
        GIT_TAG amol/deps
    )
    FetchContent_MakeAvailable(MarlinRlpx)

    add_library(Marlin::rlpx ALIAS rlpx)
endif()
