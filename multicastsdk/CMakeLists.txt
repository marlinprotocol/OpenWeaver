cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(multicastsdk VERSION 0.0.1 LANGUAGES C CXX)

if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO)
else()
	add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_library(multicastsdk INTERFACE)
add_library(marlin::multicastsdk ALIAS multicastsdk)

target_include_directories(multicastsdk
	INTERFACE
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# marlin::pubsub
target_link_libraries(multicastsdk INTERFACE marlin::pubsub)

# marlin::beacon
target_link_libraries(multicastsdk INTERFACE marlin::beacon)

# marlin::asyncio
target_link_libraries(multicastsdk INTERFACE marlin::asyncio)

# spdlog
target_link_libraries(multicastsdk INTERFACE spdlog::spdlog_header_only)

install(TARGETS multicastsdk
	EXPORT marlin-multicastsdk-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

##########################################################
# Export
##########################################################

install(EXPORT marlin-multicastsdk-export
	FILE
		MarlinMulticastSDKTargets.cmake
	NAMESPACE
		marlin::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/marlinMulticastSDK
)

install(FILES
	${CMAKE_CURRENT_LIST_DIR}/cmake/MarlinMulticastSDKConfig.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/marlinMulticastSDK
)


##########################################################
# C++ SDK examples
##########################################################

function(bridge_exe EXE_PREFIX DEFAULT_PUBSUB DEFAULT_DISC DEFAULT_LISTEN DEFAULT_NETWORK_ID DEFAULT_MASK)
	set(EXE_NAME ${EXE_PREFIX}_bridge)
	add_executable(${EXE_NAME}
		src/bridge.cpp
	)

	target_link_libraries(${EXE_NAME} PUBLIC multicastsdk)

	# MarlinLpf
	target_link_libraries(${EXE_NAME} PUBLIC marlin::lpf)

	# MarlinStream
	target_link_libraries(${EXE_NAME} PUBLIC marlin::stream)

	# structopt
	target_link_libraries(${EXE_NAME} PUBLIC structopt::structopt)

	target_compile_definitions(${EXE_NAME} PUBLIC
		MARLIN_BRIDGE_DEFAULT_PUBSUB_PORT=${DEFAULT_PUBSUB}
		MARLIN_BRIDGE_DEFAULT_DISC_PORT=${DEFAULT_DISC}
		MARLIN_BRIDGE_DEFAULT_LISTEN_PORT=${DEFAULT_LISTEN}
		MARLIN_BRIDGE_DEFAULT_NETWORK_ID=${DEFAULT_NETWORK_ID}
		MARLIN_BRIDGE_DEFAULT_MASK=${DEFAULT_MASK}
	)
endfunction()

bridge_exe(dot 20700 20702 20901 "0x9bd00430e53a5999c7c603cfc04cbdaf68bdbc180f300e4a2067937f57a0534f" All)
bridge_exe(iris 21700 21702 21901 "0x400c11d24cbc493052ef2bdd6a364730aa6ad3883b7e7d99ba40b34062cf1701" Cosmosv1)
bridge_exe(cosmos 22200 22202 22401 "0xa7afb7207c63764e77fd8c26e89627207accddd4b033d7d0cd3b669b62d6af4f" Cosmosv1)

function(sc_exe EXE_PREFIX DEFAULT_PUBSUB DEFAULT_DISC DEFAULT_LISTEN DEFAULT_NETWORK_ID DEFAULT_MASK DEFAULT_CHAIN)
	set(EXE_NAME ${EXE_PREFIX}_sc)
	add_executable(${EXE_NAME}
		src/spamchecker.cpp
	)

	target_link_libraries(${EXE_NAME} PUBLIC multicastsdk)
	target_link_libraries(${EXE_NAME} PUBLIC abci_cosmos abci_eth abci_matic)
	target_link_libraries(${EXE_NAME} PUBLIC structopt::structopt)

	target_compile_definitions(${EXE_NAME} PUBLIC
		MARLIN_SC_DEFAULT_PUBSUB_PORT=${DEFAULT_PUBSUB}
		MARLIN_SC_DEFAULT_DISC_PORT=${DEFAULT_DISC}
		MARLIN_SC_DEFAULT_LISTEN_PORT=${DEFAULT_LISTEN}
		MARLIN_SC_DEFAULT_NETWORK_ID=${DEFAULT_NETWORK_ID}
		MARLIN_SC_DEFAULT_MASK=${DEFAULT_MASK}
		MARLIN_SC_DEFAULT_CHAIN=${DEFAULT_CHAIN}
	)
endfunction()

sc_exe(dot 20700 20702 20901 "0x9bd00430e53a5999c7c603cfc04cbdaf68bdbc180f300e4a2067937f57a0534f" All cosmos)
sc_exe(iris 21700 21702 21901 "0x400c11d24cbc493052ef2bdd6a364730aa6ad3883b7e7d99ba40b34062cf1701" Cosmosv1 cosmos)
sc_exe(cosmos 22200 22202 22401 "0xa7afb7207c63764e77fd8c26e89627207accddd4b033d7d0cd3b669b62d6af4f" Cosmosv1 cosmos)
sc_exe(eth 15200 15202 15401 "0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4" All eth)
sc_exe(matic 27200 22702 22901 "0xa6a7de01e8b7ba6a4a61c782a73188d808fc1f3cf5743fadb68a02ed884b594" All matic)

function(probe_exe EXE_PREFIX DEFAULT_PUBSUB DEFAULT_DISC DEFAULT_NETWORK_ID DEFAULT_MASK)
	set(EXE_NAME ${EXE_PREFIX}_probe)
	add_executable(${EXE_NAME}
		src/probe.cpp
	)

	target_link_libraries(${EXE_NAME} PUBLIC multicastsdk)
	target_link_libraries(${EXE_NAME} PUBLIC structopt::structopt)

	target_compile_definitions(${EXE_NAME} PUBLIC
		MARLIN_PROBE_DEFAULT_PUBSUB_PORT=${DEFAULT_PUBSUB}
		MARLIN_PROBE_DEFAULT_DISC_PORT=${DEFAULT_DISC}
		MARLIN_PROBE_DEFAULT_NETWORK_ID=${DEFAULT_NETWORK_ID}
		MARLIN_PROBE_DEFAULT_MASK=${DEFAULT_MASK}
	)
endfunction()

probe_exe(eth 15000 15002 "0xaaaebeba3810b1e6b70781f14b2d72c1cb89c0b2b320c43bb67ff79f562f5ff4" All)
probe_exe(dot 20700 20702 "0x9bd00430e53a5999c7c603cfc04cbdaf68bdbc180f300e4a2067937f57a0534f" All)
probe_exe(iris 21700 21702 "0x400c11d24cbc493052ef2bdd6a364730aa6ad3883b7e7d99ba40b34062cf1701" Cosmosv1)
probe_exe(cosmos 22200 22202 "0xa7afb7207c63764e77fd8c26e89627207accddd4b033d7d0cd3b669b62d6af4f" Cosmosv1)
probe_exe(matic 22700 22702 "0xa6a7de01e8b7ba6a4a61c782a73188d808fc1f3cf5743fadb68a02ed884b594" All)

add_executable(msggen
	examples/msggen.cpp
)

target_link_libraries(msggen PUBLIC multicastsdk)

add_executable(multicastTcpBridge
	examples/MulticastTcpBridge.cpp
)

target_link_libraries(multicastTcpBridge PUBLIC multicastsdk)

# MarlinLpf
target_link_libraries(multicastTcpBridge PUBLIC marlin::lpf)

# MarlinStream
target_link_libraries(multicastTcpBridge PUBLIC marlin::stream)


##########################################################
# C SDK
##########################################################

add_library(multicastsdk_c SHARED
	src/MarlinMulticastClient.cpp
)
add_library(marlin::multicastsdk_c ALIAS multicastsdk_c)

target_include_directories(multicastsdk_c
	PUBLIC
		$<INSTALL_INTERFACE:include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(multicastsdk_c PUBLIC spdlog::spdlog_header_only)
target_link_libraries(multicastsdk_c PUBLIC multicastsdk)

target_compile_options(multicastsdk_c PRIVATE -Werror -Wall -Wextra -pedantic-errors)
target_compile_features(multicastsdk_c PUBLIC cxx_std_17)


##########################################################
# C SDK examples
##########################################################

add_executable(multicastsdk_c_example
	examples/main.c
)

target_link_libraries(multicastsdk_c_example PUBLIC multicastsdk_c)
target_compile_features(multicastsdk_c_example PUBLIC c_std_11)
