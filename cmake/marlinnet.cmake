# MarlinNet
find_package(MarlinNet CONFIG COMPONENTS net
	NAMES "Marlin" CONFIGS "MarlinNetConfig.cmake")
if(NOT MarlinNet_FOUND)
	message("Using internal MarlinNet")

	include(FetchContent)

	FetchContent_Declare(MarlinNet
		GIT_REPOSITORY https://gitlab.com/marlinprotocol/net.cpp.git
		GIT_TAG master
	)
	FetchContent_MakeAvailable(MarlinNet)	

	add_library(Marlin::net ALIAS net)
endif()