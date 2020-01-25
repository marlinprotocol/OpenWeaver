find_package(GTest QUIET)
if(NOT GTest_FOUND)
	message("-- GTest not found. Using internal GTest.")

	include(FetchContent)

	FetchContent_Declare(GTest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG release-1.8.1
	)
	FetchContent_MakeAvailable(GTest)

	add_library(GTest::GTest ALIAS gtest)
	add_library(GTest::Main ALIAS gtest_main)
else()
	message("-- GTest found. Using system GTest.")
endif()
