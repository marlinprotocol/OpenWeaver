find_package(GTest)
if(NOT GTest_FOUND)
	message("Using internal GTest")

	include(FetchContent)

	FetchContent_Declare(GTest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG release-1.8.1
	)
	FetchContent_MakeAvailable(GTest)

	add_library(GTest::GTest ALIAS gtest)
	add_library(GTest::Main ALIAS gtest_main)
endif()