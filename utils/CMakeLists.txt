cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(utils VERSION 0.0.1 LANGUAGES CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

IF(APPLE)
    # Fix linking on 10.14+. See https://stackoverflow.com/questions/54068035
    LINK_DIRECTORIES(/usr/local/lib)
ENDIF()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(utils INTERFACE)
add_library(marlin::utils ALIAS utils)

target_include_directories(utils
        INTERFACE
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        )
target_compile_features(utils INTERFACE cxx_std_17)

# spdlog
target_link_libraries(utils INTERFACE spdlog::spdlog_header_only)

# abseil
# target_link_libraries(utils INTERFACE absl::flash_hash_map)

install(TARGETS utils
        EXPORT marlin-utils-export
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-utils-export
        FILE
        	MarlinUtilsTargets.cmake
        NAMESPACE
        	marlin::
        DESTINATION
        	${CMAKE_INSTALL_LIBDIR}/cmake/marlinUtils
        )

install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinUtilsConfig.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinUtils
        )

##########################################################
# Tests
##########################################################

#enable_testing()

#set(TEST_SOURCES
#        test/testAckRanges.cpp
#        test/testCongestion.cpp
#        )

#add_custom_target(stream_tests)
#foreach(TEST_SOURCE ${TEST_SOURCES})
#    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
#    add_executable(${TEST_NAME} ${TEST_SOURCE})
#    target_link_libraries(${TEST_NAME} PUBLIC GTest::GTest GTest::Main stream)
#    target_compile_options(${TEST_NAME} PRIVATE -Werror -Wall -Wextra -pedantic-errors)
#    target_compile_features(${TEST_NAME} PRIVATE cxx_std_17)
#    add_test(${TEST_NAME} ${TEST_NAME})

#    add_dependencies(stream_tests ${TEST_NAME})
#endforeach(TEST_SOURCE)

##########################################################
# All
##########################################################

