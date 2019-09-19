find_package(MarlinPubsub)
if(NOT MarlinPubsub_FOUND)
    message("Using internal MarlinPubsub")
    include(FetchContent)
    FetchContent_Declare(MarlinPubsub
        GIT_REPOSITORY https://gitlab.com/marlinprotocol/pubsub.cpp.git
        GIT_TAG amol/deps
    )
    FetchContent_MakeAvailable(MarlinPubsub)

    add_library(Marlin::pubsub ALIAS pubsub)
endif()
