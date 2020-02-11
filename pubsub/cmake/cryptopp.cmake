find_package(cryptopp QUIET)
if(NOT cryptopp_FOUND)
	message("-- cryptopp not found. Using internal cryptopp.")

	include(FetchContent)

	FetchContent_Declare(cryptopp
		GIT_REPOSITORY https://github.com/weidai11/cryptopp.git
		GIT_TAG CRYPTOPP_8_2_0
	)
	FetchContent_MakeAvailable(cryptopp)
else()
	message("-- cryptopp found. Using system cryptopp.")
endif()
