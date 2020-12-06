cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(near VERSION 0.0.1 LANGUAGES CXX)

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

add_library(near INTERFACE)

target_include_directories(near
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_compile_features(near INTERFACE cxx_std_17)


# marlin::asyncio
target_link_libraries(near INTERFACE marlin::asyncio)

# spdlog
target_link_libraries(near INTERFACE spdlog::spdlog_header_only)

# cryptopp
target_link_libraries(near INTERFACE cryptopp::CryptoPP)

# libbase58
target_link_libraries(near INTERFACE base58::base58)

# marlin::multicastsdk
target_link_libraries(near INTERFACE marlin::multicastsdk)

# libsodium
target_link_libraries(near INTERFACE sodium::sodium)

# snappy
target_link_libraries(near INTERFACE snappy)

# boost::filesystem
target_link_libraries(near INTERFACE Boost::filesystem)

install(TARGETS near
	EXPORT marlin-near-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-near-export
	FILE
		MarlinNearTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinNear
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinNearConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinNear
)

##########################################################
# Build gateway
##########################################################

add_custom_target(near_examples)

add_executable(near_gateway
	src/near.cpp
)
add_dependencies(near_examples near_gateway)
target_link_libraries(near_gateway PUBLIC near)
target_compile_options(near_gateway PRIVATE -Werror -Wall -Wextra -pedantic-errors)


##########################################################
# All
##########################################################

add_custom_target(near_all)
add_dependencies(near_all near near_examples)
