find_package(MarlinLpf QUIET)
if(NOT MarlinLpf_FOUND)
	message("-- MarlinLpf not found. Using internal MarlinLpf.")
    include(FetchContent)
    FetchContent_Declare(MarlinLpf
        GIT_REPOSITORY https://gitlab.com/marlinprotocol/lpf.cpp.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(MarlinLpf)

    add_library(Marlin::lpf ALIAS lpf)
else()
	message("-- MarlinLpf found. Using system MarlinLpf.")
endif()
