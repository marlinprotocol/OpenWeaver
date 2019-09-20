find_package(MarlinBeacon)
if(NOT MarlinBeacon_FOUND)
    message("Using internal MarlinBeacon")
    include(FetchContent)
    FetchContent_Declare(MarlinBeacon
        GIT_REPOSITORY https://gitlab.com/marlinprotocol/beacon.cpp.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(MarlinBeacon)

    add_library(Marlin::beacon ALIAS beacon)
endif()
