cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(marlin VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

cmake_policy(SET CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

add_subdirectory(vendor)

enable_testing()
add_subdirectory(net)
add_subdirectory(beacon)
add_subdirectory(stream)
add_subdirectory(lpf)
add_subdirectory(pubsub)
add_subdirectory(multicastsdk)
add_subdirectory(relay)
add_subdirectory(goldfish)
add_subdirectory(simulator)
add_subdirectory(des)
