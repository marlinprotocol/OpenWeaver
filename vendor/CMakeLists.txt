#-------- gtest --------#
add_subdirectory(googletest)
add_library(GTest::GTest ALIAS gtest)
add_library(GTest::Main ALIAS gtest_main)


#-------- libuv --------#
add_subdirectory(libuv)
add_library(libuv::uv_a ALIAS uv_a)

# Avert your eyes
set_property(TARGET uv_a PROPERTY INTERFACE_INCLUDE_DIRECTORIES
	$<INSTALL_INTERFACE:include>
	$<BUILD_INTERFACE:${libuv_SOURCE_DIR}/include>
)

find_package(Threads REQUIRED COMPONENTS Threads)
target_link_libraries(uv_a Threads::Threads)

install(TARGETS uv_a
	EXPORT libuv-export
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT libuv-export
	FILE
		LibuvTargets.cmake
	NAMESPACE
		libuv::
	DESTINATION
		${CMAKE_INSTALL_LIBDIR}/cmake/libuv
)


#-------- spdlog --------#
set(SPDLOG_MASTER_PROJECT ON)
add_subdirectory(spdlog)


#-------- libsodium --------#
include(ExternalProject)
ExternalProject_Add(libsodium
	SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/libsodium
	CONFIGURE_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/libsodium && ./autogen.sh && ./configure --disable-pie --prefix=${CMAKE_CURRENT_BINARY_DIR}/libsodium CFLAGS=-fPIC
	BUILD_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/libsodium && make -j8
	INSTALL_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/libsodium && make install
)

add_library(sodium STATIC IMPORTED GLOBAL)
add_library(sodium::sodium ALIAS sodium)
add_dependencies(sodium libsodium)

set_target_properties(sodium PROPERTIES
	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libsodium/lib/libsodium.a
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/libsodium/include
)


#-------- cryptopp --------#
include(ExternalProject)
ExternalProject_Add(libcryptopp
	SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/cryptopp
	CONFIGURE_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/cryptopp
	BUILD_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/cryptopp && make -j8
	INSTALL_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/cryptopp && make install PREFIX=${CMAKE_CURRENT_BINARY_DIR}/cryptopp
)

add_library(cryptopp STATIC IMPORTED GLOBAL)
add_library(cryptopp::CryptoPP ALIAS cryptopp)
add_dependencies(cryptopp libcryptopp)

set_target_properties(cryptopp PROPERTIES
	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/cryptopp/lib/libcryptopp.a
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/cryptopp/include
)


#-------- secp256k1 --------#
include(ExternalProject)
ExternalProject_Add(libsecp256k1
	SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/secp256k1
	CONFIGURE_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/secp256k1 && ./autogen.sh && ./configure --with-bignum=no --enable-module-recovery --enable-static --disable-pie --prefix=${CMAKE_CURRENT_BINARY_DIR}/secp256k1 CFLAGS=-fPIC
	BUILD_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/secp256k1 && make -j8
	INSTALL_COMMAND cd ${CMAKE_CURRENT_LIST_DIR}/secp256k1 && make install
)

add_library(secp256k1 STATIC IMPORTED GLOBAL)
add_library(secp256k1::secp256k1 ALIAS secp256k1)
add_dependencies(secp256k1 libsecp256k1)

set_target_properties(secp256k1 PROPERTIES
	IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/secp256k1/lib/libsecp256k1.a
	INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR}/secp256k1/include
)


#-------- boost --------#
set(BOOST_ENABLE_CMAKE ON)
add_subdirectory(boost)


#-------- snappy --------#
add_subdirectory(snappy)
